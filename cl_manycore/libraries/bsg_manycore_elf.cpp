#ifndef COSIM
#include <bsg_manycore_elf.h>
#else
#include "bsg_manycore_elf.h"
#endif

#include <map>
#include <string>
#include <memory>

using std::string;
using std::map;
using std::unique_ptr;

struct SymbolInfo {        
        char * name;
        Elf32_Addr   value;
        Elf32_Word   size;
        SymbolInfo(const char*name, Elf32_Addr value, Elf32_Word size):
                name(strdup(name)),
                value(value),
                size(size) {}
        virtual ~SymbolInfo() { free(name); }
};

typedef unique_ptr<SymbolInfo> SymbolInfoPtr;
typedef map<string, SymbolInfoPtr> symbol_table;

#define pr_err(fmt, ...)                                \
        fprintf(stderr, "error: " fmt, ##__VA_ARGS__);



static int object_symbol_table_import_symbols(symbol_table& symbols, unsigned char *object_data, Elf32_Shdr *symtab_shdr, Elf32_Shdr *strtab_shdr)
{
        Elf32_Word sym_n = symtab_shdr->sh_size/symtab_shdr->sh_entsize;
        Elf32_Word sym_i;
        Elf32_Sym *sym, *sym_base = (Elf32_Sym *)&object_data[symtab_shdr->sh_offset];
        char *strtab_base = (char*) &object_data[strtab_shdr->sh_offset];
        
        const char *sym_name;
        
        for (sym_i = 0; sym_i < sym_n; sym_i++) {
                sym = &sym_base[sym_i];

                /* skip symbols with no name */
                if (sym->st_name == 0)
                        continue;
                
                sym_name = &strtab_base[sym->st_name];
                
                SymbolInfo *sym_info = new SymbolInfo(sym_name, sym->st_value, sym->st_size);
                
                symbols[string(sym_name)] = SymbolInfoPtr(sym_info);
        }
        return 0;
}

static void object_symbol_table_init(const char *fname, symbol_table& symbols)
{
        FILE *f = fopen(fname, "rb");
        long size;
        unsigned char *object_data;
        Elf32_Ehdr *ehdr;
        Elf32_Shdr *shdr, *symtab_shdr, *strtab_shdr;
        Elf32_Sym  *sym;
        int section_i, r;
        
        if (!f) {
                pr_err("Failed to open '%s': %s\n", fname, strerror(errno));
                goto fail_return;
        }

        fseek(f, 0, SEEK_END);
        size = ftell(f);
        if (size < 0) {
                pr_err("Failed to stat '%s': %s\n", fname, strerror(errno));
                goto fail_close_f;
        }

        object_data = (unsigned char*)malloc(size);
        if (!object_data) {
                pr_err("Failed to read '%s': %s\n", fname, strerror(errno));
                goto fail_close_f;
        }

        fseek(f, 0, SEEK_SET);
        if ((r = fread(object_data, size, 1, f)) != 1) {
                pr_err("Failed to read '%s' (fread returned %d): %s\n", fname, r, strerror(errno));
                goto fail_free_object_data;
        }

        /* check that this is indeed an ELF file */
        if (memcmp(object_data, ELFMAG, SELFMAG) != 0) {
                pr_err("'%s' is not a valid ELF file\n", fname);
                goto fail_free_object_data;
        }
        /* check that this is 32-bit little endian */
        if (!(object_data[EI_CLASS] == ELFCLASS32  &&
              object_data[EI_DATA]  == ELFDATA2LSB)) {
                pr_err("'%s' is not a 32-bit little endian object file\n", fname);
                goto fail_free_object_data;
        }
        /* check here for RISC-V? */
        /* find each section that is a symbol table */
        ehdr = (Elf32_Ehdr*)object_data;
        shdr = (Elf32_Shdr*)&object_data[ehdr->e_shoff];
        for (section_i = 0; section_i < ehdr->e_shnum; section_i++) {
                // only looking for symbol tables
                if (shdr[section_i].sh_type != SHT_SYMTAB)
                        continue;
                
                symtab_shdr = &shdr[section_i];
                strtab_shdr = &shdr[symtab_shdr->sh_link];
                /* add all symbols to the symbol table */
                if (object_symbol_table_import_symbols(symbols, object_data, symtab_shdr, strtab_shdr) < 0)
                        goto fail_free_object_data;
        }

        free(object_data);
        fclose(f);
        return;
        
fail_free_object_data:
        free(object_data);
fail_close_f:
        fclose(f);
fail_return:
        exit(1);
}

int symbol_to_eva(const char *fname, const char *sym_name, eva_t* eva)
{
	static symbol_table symbols;
        object_symbol_table_init(fname, symbols);
	symbol_table::iterator it = symbols.find(string(sym_name));
        if (it != symbols.end()) {
                *eva = (it->second->value);
                return HB_MC_SUCCESS;
        } else {
                return HB_MC_FAIL;
        }
}
