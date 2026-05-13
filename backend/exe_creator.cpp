#include "backend.h"

#include <cstdio>

DWORD EXIT_ADDR             = 0;
DWORD GET_NUMBER_ADDR       = 0;
DWORD PRINT_NUMBER_ADDR     = 0;
DWORD SOCK_INIT_ADDR        = 0;
DWORD SOCK_TCP_CONNECT_ADDR = 0;
DWORD SOCK_CLOSE_ADDR       = 0;
DWORD SOCK_SEND_INT_ADDR    = 0;
DWORD SOCK_RECV_INT_ADDR    = 0;

static bool elem_needs_winsock_imports (const bin_tree_elem *e)
{
    if (e == nullptr)
        return false;

    if (e->type == FUNC)
    {
        const int v = static_cast<int>(e->value);
        if (v >= SOCK_INIT && v <= SOCK_RECV_INT)
            return true;
    }

    return elem_needs_winsock_imports(e->left) || elem_needs_winsock_imports(e->right);
}

static bool tree_needs_winsock_imports (const bin_tree *tree)
{
    return tree != nullptr && elem_needs_winsock_imports(tree->root);
}

bool create_exe (bin_tree *tree, FILE *exe_file, variables *var, size_t bss_virtual_size)
{
    assert(tree);
    assert(exe_file);
    assert(var);

    const size_t stub_size = SIZE_RAW;
    BYTE zeros[stub_size] = { 0 };
    size_t ptr_raw = 0x400;

    // --- ALL HEADERS:

    IMAGE_DOS_HEADER dos_header = { 0 };
    fill_dos_header(&dos_header);

    IMAGE_NT_HEADERS32 NT_header  = { 0 };
    fill_NT_header(&NT_header, 4, bss_virtual_size);

    IMAGE_SECTION_HEADER text_section = { 0 };
    fill_section_header(&text_section, ".text", VRT_SIZE, ENTRY_POINT_ADDR, SIZE_RAW, ptr_raw, CODE_CHARACTER);

    IMAGE_SECTION_HEADER import_data_section = { 0 };
    fill_section_header(&import_data_section, ".idata", VRT_SIZE, IMPORT_START, SIZE_RAW, ptr_raw + SIZE_RAW, IDATA_CHARACTER);

    IMAGE_SECTION_HEADER data_section = { 0 };
    fill_section_header(&data_section, ".data", VRT_SIZE, DATA_START, SIZE_RAW, ptr_raw + 2 * SIZE_RAW, DATA_CHARACTER);

    IMAGE_SECTION_HEADER bss_section = { 0 };
    fill_section_header(&bss_section, ".bss", bss_virtual_size, BSS_START, 0, 0, BSS_CHARACTER);

    fwrite(&dos_header, sizeof(IMAGE_DOS_HEADER), 1, exe_file);
    fwrite(&DOS_STUB,   sizeof(DOS_STUB[0]), sizeof(DOS_STUB) / sizeof(DOS_STUB[0]), exe_file);  //TODO 1 fwrite
    fwrite(&NT_header,  sizeof(IMAGE_NT_HEADERS32), 1, exe_file);

    fwrite(&text_section,        sizeof(IMAGE_SECTION_HEADER), 1, exe_file);
    fwrite(&import_data_section, sizeof(IMAGE_SECTION_HEADER), 1, exe_file);
    fwrite(&data_section,        sizeof(IMAGE_SECTION_HEADER), 1, exe_file);
    fwrite(&bss_section,        sizeof(IMAGE_SECTION_HEADER), 1, exe_file);

    size_t sizeof_headers = sizeof(IMAGE_DOS_HEADER) + sizeof(DOS_STUB) + sizeof(IMAGE_NT_HEADERS32) + 4 * sizeof(IMAGE_SECTION_HEADER);
    fwrite(zeros, sizeof(zeros[0]), ptr_raw - sizeof_headers, exe_file); // for alignment (1024 bytes of all headers)

    // --- END OF HEADERS

    const bool link_sock = tree_needs_winsock_imports(tree);

    import_table imp_table;
    imp_table.fill_default(link_sock);

    EXIT_ADDR         = IMAGE_BASE + IMPORT_START + imp_table.get_proc_addr(0);
    GET_NUMBER_ADDR   = IMAGE_BASE + IMPORT_START + imp_table.get_proc_addr(1);
    PRINT_NUMBER_ADDR = IMAGE_BASE + IMPORT_START + imp_table.get_proc_addr(2);

    if (link_sock)
    {
        fprintf(stderr,
                "lang_compile: this executable imports Winsock (8 symbols from sfasmlib.dll).\n"
                "lang_compile: use sfasmlib_runtime.c — build_runtime.bat (MinGW i686) or "
                "build_runtime_msvc.bat (Visual Studio x86). A 3-export MASM DLL will not load.\n");

        SOCK_INIT_ADDR        = IMAGE_BASE + IMPORT_START + imp_table.get_proc_addr(3);
        SOCK_TCP_CONNECT_ADDR = IMAGE_BASE + IMPORT_START + imp_table.get_proc_addr(4);
        SOCK_CLOSE_ADDR       = IMAGE_BASE + IMPORT_START + imp_table.get_proc_addr(5);
        SOCK_SEND_INT_ADDR    = IMAGE_BASE + IMPORT_START + imp_table.get_proc_addr(6);
        SOCK_RECV_INT_ADDR    = IMAGE_BASE + IMPORT_START + imp_table.get_proc_addr(7);
    }
    else
    {
        SOCK_INIT_ADDR        = 0;
        SOCK_TCP_CONNECT_ADDR = 0;
        SOCK_CLOSE_ADDR       = 0;
        SOCK_SEND_INT_ADDR    = 0;
        SOCK_RECV_INT_ADDR    = 0;
    }

    // --- PROGRAM SECTIONS:

    // 1. TEXT SECTION

    BYTE *opcodes = (BYTE*) calloc(SIZE_RAW, sizeof(BYTE));
    fill_text_sec(tree, opcodes, var);

    fwrite((unsigned char*) opcodes, sizeof(opcodes[0]), stub_size, exe_file);

    // 2. IMPORT DATA SECTION
    
    imp_table.put_in_file(exe_file);

    // 3. DATA SECTION
    fwrite(zeros, sizeof(zeros[0]), stub_size, exe_file);

    // --- EVERYTHING IS WRITTEN

    free(opcodes);

    return true;
}

void fill_dos_header (IMAGE_DOS_HEADER *dos_header)
{
    dos_header->e_magic    = 'ZM';      // Mark Zbikowski

    dos_header->e_cblp     = 0x0090;    
    dos_header->e_cp       = 0x0003;
    dos_header->e_cparhdr  = 0x0004;    // 64 bytes (size of DOS Header)

    dos_header->e_minalloc = 0x0010;
    dos_header->e_maxalloc = 0xFFFF;

    dos_header->e_sp       = 0x00B8;
    dos_header->e_lfarlc   = 0x0040;

    dos_header->e_lfanew   = 0x00B0;    // The beginning of NT Header
}

void fill_NT_header (IMAGE_NT_HEADERS32 *NT_header, int NUM_OF_SEC, size_t bss_virtual_size)
{
    NT_header->Signature = 'EP';

    fill_NT_file_header(&(NT_header->FileHeader), NUM_OF_SEC);
    fill_NT_optional_header(&(NT_header->OptionalHeader), bss_virtual_size);
}

void fill_NT_file_header (IMAGE_FILE_HEADER *NT_file_header, int NUM_OF_SEC)
{
    NT_file_header->Machine              = IMAGE_FILE_MACHINE_I386;
    NT_file_header->NumberOfSections     = static_cast<WORD>(NUM_OF_SEC);
    NT_file_header->TimeDateStamp        = static_cast<DWORD>(time(nullptr));
    NT_file_header->SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER32);
    NT_file_header->Characteristics      = IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_32BIT_MACHINE;
}

void fill_NT_optional_header (IMAGE_OPTIONAL_HEADER32 *NT_optional_header, size_t bss_virtual_size)
{
    NT_optional_header->Magic                 = IMAGE_NT_OPTIONAL_HDR32_MAGIC;

    NT_optional_header->AddressOfEntryPoint   = ENTRY_POINT_ADDR;
    NT_optional_header->ImageBase             = IMAGE_BASE;
    NT_optional_header->BaseOfCode            = ENTRY_POINT_ADDR;
    NT_optional_header->BaseOfData            = DATA_START;
    NT_optional_header->SectionAlignment      = ENTRY_POINT_ADDR;

    size_t image_end = bss_virtual_size > 0 ? (BSS_START + bss_virtual_size) : (SIZE_RAW + 3 * VRT_SIZE);
    size_t align     = ENTRY_POINT_ADDR;

    NT_optional_header->SizeOfImage =
        static_cast<DWORD>((image_end + align - 1U) / align * align);

    NT_optional_header->FileAlignment         = 0x200;
    NT_optional_header->SizeOfHeaders         = 0x400;

    NT_optional_header->MajorSubsystemVersion = 4;
    NT_optional_header->Subsystem             = IMAGE_SUBSYSTEM_WINDOWS_CUI;
    NT_optional_header->NumberOfRvaAndSizes   = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;
    NT_optional_header->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = IMPORT_START;
    NT_optional_header->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size =
        static_cast<DWORD>(sizeof(IMAGE_IMPORT_DESCRIPTOR) * IMPORT_DESCRIPTOR_COUNT);
}

void fill_section_header (IMAGE_SECTION_HEADER *section, const char sec_name[], size_t vrt_size, size_t vrt_addr,
                         size_t size_raw_data, size_t ptr_raw_data, size_t character)
{
    sprintf((char *const) section->Name, "%s", sec_name);
    section->Misc.VirtualSize = vrt_size;

    section->VirtualAddress   = vrt_addr;
    section->SizeOfRawData    = size_raw_data;
    section->PointerToRawData = ptr_raw_data;
    section->Characteristics  = character;
}
