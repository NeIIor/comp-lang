#ifndef IMPORT_TABLE_H_INCLUDED
#define IMPORT_TABLE_H_INCLUDED

#include "exe_config.h"

struct import_name
{
    WORD Hint;
    const CHAR* Name;
    size_t size;

    import_name() :
        Hint(0),
        Name(nullptr),
        size(0)
    {}

    import_name(const char name[], size_t size, WORD hnt = 0) :
        Hint(hnt),
        Name(name),
        size(size)
    {}

    void fill(const char name[], size_t size, WORD hnt = 0)
    {
        Name = name;
        this->size = size;
        Hint = hnt;
    }

    size_t put_in_file(FILE* file)
    {
        if (file == nullptr)
            return 0;

        fwrite(&Hint, sizeof(WORD), 1, file);
        fwrite(Name, sizeof(char), size, file);

        char zero = 0;
        fwrite(&zero, sizeof(zero), 1, file);

        return size;
    }
};

class import_table
{
private:
    static const size_t N_IMPORTS = 3;

    IMAGE_IMPORT_DESCRIPTOR* table;
    import_name              name_imp[N_IMPORTS];
    IMAGE_THUNK_DATA32       thunk[N_IMPORTS + 1];
    size_t                   names_size;

public:
    import_table(void) :
        table(nullptr),
        names_size(0)
    {
        table = (IMAGE_IMPORT_DESCRIPTOR*) calloc(IMPORT_DESCRIPTOR_COUNT, sizeof(IMAGE_IMPORT_DESCRIPTOR));
    }

    void fill_def_names(void)
    {
        name_imp[0].fill("ExitProgram", 11);
        name_imp[1].fill("GetNumber", 9);
        name_imp[2].fill("PrintNumber", 11);

        names_size = 0;
        for (size_t i = 0; i < N_IMPORTS; ++i)
            names_size += name_imp[i].size + 3;
    }

    bool fill_default(void)
    {
        fill_def_names();

        const size_t desc_size = sizeof(IMAGE_IMPORT_DESCRIPTOR) * IMPORT_DESCRIPTOR_COUNT;
        const size_t int_size  = sizeof(IMAGE_THUNK_DATA32) * (N_IMPORTS + 1);

        const size_t names_start_rva = IMPORT_START + desc_size + int_size;
        const size_t iat_start_rva   = names_start_rva + names_size;
        const size_t dll_name_rva    = iat_start_rva + int_size;

        table[0].OriginalFirstThunk = static_cast<DWORD>(IMPORT_START + desc_size);
        table[0].FirstThunk         = static_cast<DWORD>(iat_start_rva);
        table[0].Name               = static_cast<DWORD>(dll_name_rva);

        size_t cur_name_size = 0;
        for (size_t i = 0; i < N_IMPORTS; ++i)
        {
            thunk[i].u1.AddressOfData = static_cast<DWORD>(names_start_rva + cur_name_size);
            cur_name_size += name_imp[i].size + 3;
        }
        thunk[N_IMPORTS].u1.AddressOfData = 0;

        return true;
    }

    bool put_in_file(FILE* file)
    {
        if (file == nullptr)
            return false;

        const size_t dll_name_size = 13;
        const size_t int_size    = sizeof(IMAGE_THUNK_DATA32) * (N_IMPORTS + 1);
        const size_t desc_size   = sizeof(IMAGE_IMPORT_DESCRIPTOR) * IMPORT_DESCRIPTOR_COUNT;
        const size_t total_data  = desc_size + int_size + names_size + int_size + dll_name_size;
        const size_t stub_size   = 1 + SIZE_RAW - total_data;
        char* stub = new char[stub_size] {0};

        fwrite(table, sizeof(table[0]), IMPORT_DESCRIPTOR_COUNT, file);

        fwrite(thunk, sizeof(thunk[0]), N_IMPORTS + 1, file);

        for (size_t i = 0; i < N_IMPORTS; ++i)
            name_imp[i].put_in_file(file);

        fwrite(thunk, sizeof(thunk[0]), N_IMPORTS + 1, file);

        fprintf(file, "sfasmlib.dll\0");

        fwrite(stub, sizeof(stub[0]), stub_size, file);

        delete[] stub;

        return true;
    }

    unsigned get_proc_addr(size_t num)
    {
        const size_t desc_size = sizeof(IMAGE_IMPORT_DESCRIPTOR) * IMPORT_DESCRIPTOR_COUNT;
        const size_t int_size  = sizeof(IMAGE_THUNK_DATA32) * (N_IMPORTS + 1);

        return static_cast<unsigned>(desc_size + int_size + names_size + num * sizeof(IMAGE_THUNK_DATA32));
    }

    ~import_table(void)
    {
        if (table != nullptr)
        {
            free(table);
            table = nullptr;
        }
    }
};


#endif // IMPORT_TABLE_H_INCLUDED
