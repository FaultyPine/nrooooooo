#include "main.h"

#include <string.h>
#include <time.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <list>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <thread>
#include <elf.h>
#include "qemu_elf.h"
#include <cxxabi.h>
#include "crc32.h"
#include "uc_inst.h"
#include "logging.h"
#include "constants.h"
#include "clustermanager.h"
#include "eh.h"
#include "lua_transpile.h"
#include <useful.h>

#define MAX_CLUSTERS_ACTIVE 100

std::atomic<int> clusters_active = 0;
int imports_size = 0;
std::map<std::string, uint64_t> unresolved_syms;
std::map<uint64_t, std::string> unresolved_syms_rev;
std::map<std::string, uint64_t> resolved_syms;
std::map<uint64_t, std::string> resolved_syms_rev;
std::map<std::pair<uint64_t, uint64_t>, uint64_t> function_hashes;

std::map<std::string, uint64_t> l2cagents;
std::map<uint64_t, std::string> l2cagents_rev;

bool syms_scanned = false;
bool trace_code = true;

struct nso_header
{
    uint32_t start;
    uint32_t mod;
};

struct mod0_header
{
    uint32_t magic;
    int32_t dynamic;
    int32_t bss_start;
    int32_t bss_end;
    int32_t unwind_start;
    int32_t unwind_end;
};

void nro_assignsyms(void* base)
{
    const Elf64_Dyn* dyn = NULL;
    const Elf64_Sym* symtab = NULL;
    const char* strtab = NULL;
    uint64_t numsyms = 0;
    
    if (syms_scanned) return;
    
    struct nso_header* header = (struct nso_header*)base;
    struct mod0_header* modheader = (struct mod0_header*)(base + header->mod);
    dyn = (const Elf64_Dyn*)(base + header->mod + modheader->dynamic);
    
    //parse_eh(base, header->mod + modheader->unwind_start);
    
    for (; dyn->d_tag != DT_NULL; dyn++)
    {
        switch (dyn->d_tag)
        {
            case DT_SYMTAB:
                symtab = (const Elf64_Sym*)(base + dyn->d_un.d_ptr);
                break;
            case DT_STRTAB:
                strtab = (const char*)(base + dyn->d_un.d_ptr);
                break;
        }
    }
    
    numsyms = ((uintptr_t)strtab - (uintptr_t)symtab) / sizeof(Elf64_Sym);
    
    for (uint64_t i = 0; i < numsyms; i++)
    {
        char* demangled = abi::__cxa_demangle(strtab + symtab[i].st_name, 0, 0, 0);

        if (symtab[i].st_shndx == 0 && demangled)
        {
            //TODO: just read the main NSO for types/sizes? Or have them resolve to the main NSO

            uint64_t import_size = 0x8;
            std::string demangled_str = std::string(demangled);
            if (demangled_str == "phx::detail::CRC32Table::table_")
            {
                import_size = sizeof(crc32_tab);
            }
            else if (demangled_str == "lib::L2CValue::NIL")
            {
                import_size = 0x10;
            }
            else if (!strncmp(demangled, "`vtable for'", 12))
            {
                import_size = 0x1000;
            }
            else if (demangled_str == "lib::Singleton<app::BattleObjectWorld>::instance_"
                     || demangled_str == "lib::Singleton<app::FighterManager>::instance_"
                     || demangled_str == "lib::Singleton<app::BattleObjectManager>::instance_"
                     || demangled_str == "lib::Singleton<app::FighterCutInManager>::instance_")
            {
                import_size = 0x100;
            }
            
            uint64_t addr = IMPORTS + (imports_size + import_size);
            unresolved_syms[std::string(demangled_str)] = addr;
            unresolved_syms_rev[addr] = std::string(demangled);
            
            imports_size += import_size;
        }
        else if (symtab[i].st_shndx && demangled)
        {
            resolved_syms[std::string(demangled)] = NRO + symtab[i].st_value;
            resolved_syms_rev[NRO + symtab[i].st_value] = std::string(demangled);
        }
        else
        {

        }
        free(demangled);
    }
    
    syms_scanned = true;
}

void nro_relocate(void* base)
{
    const Elf64_Dyn* dyn = NULL;
    const Elf64_Rela* rela = NULL;
    const Elf64_Sym* symtab = NULL;
    const char* strtab = NULL;
    uint64_t relasz = 0;
    uint64_t numsyms = 0;
    
    struct nso_header* header = (struct nso_header*)base;
    struct mod0_header* modheader = (struct mod0_header*)(base + header->mod);
    dyn = (const Elf64_Dyn*)((void*)modheader + modheader->dynamic);

    for (; dyn->d_tag != DT_NULL; dyn++)
    {
        switch (dyn->d_tag)
        {
            case DT_SYMTAB:
                symtab = (const Elf64_Sym*)(base + dyn->d_un.d_ptr);
                break;
            case DT_STRTAB:
                strtab = (const char*)(base + dyn->d_un.d_ptr);
                break;
            case DT_RELA:
                rela = (const Elf64_Rela*)(base + dyn->d_un.d_ptr);
                break;
            case DT_RELASZ:
                relasz += dyn->d_un.d_val / sizeof(Elf64_Rela);
                break;
            case DT_PLTRELSZ:
                relasz += dyn->d_un.d_val / sizeof(Elf64_Rela);
                break;
        }
    }
    
    if (rela == NULL)
    {
        return;
    }

    for (; relasz--; rela++)
    {
        uint32_t sym_idx = ELF64_R_SYM(rela->r_info);
        const char* name = strtab + symtab[sym_idx].st_name;

        uint64_t sym_val = (uint64_t)base + symtab[sym_idx].st_value;
        if (!symtab[sym_idx].st_value)
            sym_val = 0;

        switch (ELF64_R_TYPE(rela->r_info))
        {
            case R_AARCH64_RELATIVE:
            {
                uint64_t* ptr = (uint64_t*)(base + rela->r_offset);
                *ptr = NRO + rela->r_addend;
                break;
            }
            case R_AARCH64_GLOB_DAT:
            case R_AARCH64_JUMP_SLOT:
            case R_AARCH64_ABS64:
            {
                uint64_t* ptr = (uint64_t*)(base + rela->r_offset);
                char* demangled = abi::__cxa_demangle(name, 0, 0, 0);
                
                if (demangled)
                {
                    //printf("@ %" PRIx64 ", %s -> %" PRIx64 ", %" PRIx64 "\n", NRO + rela->r_offset, demangled, unresolved_syms[std::string(demangled)], *ptr);
                    if (resolved_syms[std::string(demangled)])
                        *ptr = resolved_syms[std::string(demangled)];
                    else
                        *ptr = unresolved_syms[std::string(demangled)];
                    free(demangled);
                }
                break;
            }
            default:
            {
                printf("Unknown relocation type %" PRId32 "\n", ELF64_R_TYPE(rela->r_info));
                break;
            }
        }
    }
}

uint64_t hash40(const void* data, size_t len)
{
    return crc32(data, len) | (len & 0xFF) << 32;
}

typedef struct cluster_struct
{
    std::string agent_name;
    std::string func_name;
    std::string outdir;
    uint64_t funcptr;
} cluster_struct;

void transpile_thread(LuaTranspiler* transpiler)
{
    delete transpiler;
}

void cluster_oncomplete(ClusterManager* cluster, uint64_t ret, void* data)
{
    char tmp[256];
    std::string out = "";
    cluster_struct* vals = (cluster_struct*)data;
    
    std::string agent_name = vals->agent_name;
    std::string func_name = vals->func_name;
    std::string outdir = vals->outdir;
    uint64_t funcptr = vals->funcptr;

    out += ">--------------------------------------<\n";
    
    int spaces = 20 - (agent_name.length() / 2);
    if (spaces < 0) spaces = 0;
    for (int i = 0; i < spaces; i++)
    {
        out += " ";
    }
    out += agent_name + "\n";
    
    spaces = 20 - (func_name.length() / 2);
    if (spaces < 0) spaces = 0;
    for (int i = 0; i < spaces; i++)
    {
        out += " ";
    }
    out += func_name + "\n";

    std::string dir_out = outdir + "/" + agent_name;
    std::string file_out = dir_out + "/" + func_name + ".txt";
    std::string file_out_lua = dir_out + "/" + func_name + ".lc";
    std::filesystem::create_directories(dir_out);
    std::ofstream file(file_out);
    
    snprintf(tmp, 255, "                %8" PRIx64 "\n", cluster->block_hash(funcptr));
    out += std::string(tmp);
    out += ">--------------------------------------<\n";

    file << out;

    try {
        cluster->print_blocks(funcptr, file);
    } catch (std::exception& e) {
        std::cout << "Failed to write blocks with exception:" << std::endl;
        std::cout << e.what() << std::endl;
    }

    file << "<-------------------------------------->\n";
    
    clusters_active--;
    
    std::thread transpile(transpile_thread, new LuaTranspiler(file_out_lua, cluster->tokens, funcptr));
    transpile.detach();
    
    delete vals;
    delete cluster;
}

void cluster_work(ClusterManager* cluster, std::string character, std::string outdir, uint64_t l2cagent, uint64_t funcptr, uint64_t hash)
{
    std::string out = "";

    out += ">--------------------------------------<\n";
    
    std::string agent_name = l2cagents_rev[l2cagent];
    
    std::string func_name = "";
    if (unhash[hash].length() != 0)
    {
        func_name = unhash[hash];
    }
    else if (status_funcs[hash].length() != 0)
    {
        func_name = status_funcs[hash];
    }
    
    if (func_name.length() == 0)
    {
        char tmp[256];
        snprintf(tmp, 255, "%" PRIx64, hash);
        func_name = std::string(tmp);
    }

    while (clusters_active >= MAX_CLUSTERS_ACTIVE || uc_insts_active > (MAX_CLUSTERS_ACTIVE - 10))
    {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(10ms);
    }

    printf("%s/%s %zx %" PRIx64 " %" PRIx64 "\n", agent_name.c_str(), func_name.c_str(), func_name.length(), funcptr, hash);
    
    cluster_struct* vals = new cluster_struct;
    vals->agent_name = agent_name;
    vals->func_name = func_name;
    vals->outdir = outdir;
    vals->funcptr = funcptr;
    
    /*std::thread* t = cluster->execute_threaded(funcptr, cluster_oncomplete, vals, true, true, l2cagent, 0xFFFA000000000000);
    t->join();
    delete t;*/
    
    uint64_t x1, x2;
    
    
    ClusterManager* clone = new ClusterManager(cluster);
    clusters_active++;

    // if (func_name.find("STATUS_MAIN") != std::string::npos) {
    //     uc_mem_write(
    //         clone->get_uc(), 
    //         funcptr + 4,
    //         "\x1f\x20\x03\xd5",
    //         4);
    // }
    
    if (!strncmp(agent_name.c_str() + character.size() + 1, "ai_mode", 7))
    {
        //TODO: some sorta registration for these input vars
        x1 = clone->heap_alloc(0x10);
        x2 = clone->heap_alloc(0x10);
    }
    else
    {
        x1 = 0xFFFA000000000000;
        x2 = 0xFFFA000000000000;
    }
    
    std::thread* t = clone->execute_threaded(funcptr, cluster_oncomplete, vals, true, true, l2cagent, x1, x2);
    t->detach();
    delete t;
}

int main(int argc, char **argv, char **envp)
{
    char tmp[256];
    uint64_t x0, x1, x2, x3;
    x1 = 0xFFFE000000000000; // BattleObject
    x2 = 0xFFFD000000000000; // BattleObjectModuleAccessor
    x3 = 0xFFFC000000000000; // lua_state
    
    if (argc < 3)
    {
        printf("Usage: %s <lua2cpp_char.nro> <outdir>\n", argv[0]);
        return -1;
    }

    init_character_objects();
    init_const_value_table();
    
    // Load in unhashed strings
    std::ifstream strings("hashstrings_lower.txt");    
    std::string line;
    while (std::getline(strings, line))
    {
        uint64_t crc = hash40((const void*)line.c_str(), strlen(line.c_str()));
        unhash[crc] = line;
    }
    
    ClusterManager cluster = ClusterManager(std::string(argv[1]));
    
    // Scan exports to find the character name
    std::string character = "";
    for (auto& pair : resolved_syms)
    {
        std::string func = pair.first;
        char* match = "lua2cpp::create_agent_fighter_status_script_";
        
        
        if (!strncmp(func.c_str(), match, strlen(match)))
        {
            for (int i = strlen(match); i < func.length(); i++)
            {
                if (func[i] == '(') break;
                character += func[i];
            }
            break;
        }
    }
    
    logmask_unset(LOGMASK_DEBUG | LOGMASK_INFO);
    // logmask_set(LOGMASK_VERBOSE);

    uint32_t babe_indices[CONST_VALUE_TABLE_SIZE];
    for (size_t i = 0; i < CONST_VALUE_TABLE_SIZE; i++) {
        babe_indices[i] = i | 0xBABE0000;
    }

    if (unresolved_syms["lua2cpp::L2CAgentGeneratedBase::const_value_table__"])
        memcpy(cluster.uc_ptr_to_real_ptr(unresolved_syms["lua2cpp::L2CAgentGeneratedBase::const_value_table__"]), babe_indices, sizeof(babe_indices));
    else
        memcpy(cluster.uc_ptr_to_real_ptr(resolved_syms["lua2cpp::L2CAgentGeneratedBase::const_value_table__"]), babe_indices, sizeof(babe_indices));

    for (auto& agent : agents)
    {
        for (auto& object : character_objects[character])
        {
            std::string hashstr = object;
            std::string key = hashstr + "_" + agent;
            std::string func = "lua2cpp::create_agent_fighter_" + agent + "_" + character;
            std::string args = "(phx::Hash40, app::BattleObject*, app::BattleObjectModuleAccessor*, lua_State*)";
            
            x0 = hash40(hashstr.c_str(), hashstr.length()); // Hash40
            uint64_t output;
            void* l2c_fighter;
            if (agent == "status_script" && character == "common") {
                cluster.add_import_hook(resolved_syms["lua2cpp::L2CAgentBase::sv_set_status_func(lib::L2CValue const&, lib::L2CValue const&, void*)"]);
                cluster.add_import_hook(resolved_syms["lua2cpp::L2CAgentBase::sv_copy_status_func(lib::L2CValue const&, lib::L2CValue const&, lib::L2CValue const&)"]);
    
                func = "lua2cpp::L2CFighterCommon::sub_set_fighter_common_table";
                args = "()";

                x0 = cluster.heap_alloc(0x1000);
                // NOP random member funcs
                uint64_t to_nop[8] = {
                    0x1002d186c, 0x1002d1870, 0x1002d1874, 0x1002d1878,
                    0x1002d18bc, 0x1002d18c0, 0x1002d18c4, 0x1002d18c8,
                };
                uint32_t ret_asm = INSTR_RET;
                for (auto nop_addr : to_nop) {
                    uc_mem_write(
                        cluster.get_uc(), 
                        nop_addr,
                        "\x1f\x20\x03\xd5",
                        4);
                }
                
                // stub status func setter
                uc_mem_write(
                    cluster.get_uc(), 
                    resolved_syms["lua2cpp::L2CAgentBase::sv_set_status_func(lib::L2CValue const&, lib::L2CValue const&, void*)"],
                    &ret_asm,
                    4);
                uc_mem_write(
                    cluster.get_uc(), 
                    resolved_syms["lua2cpp::L2CAgentBase::sv_copy_status_func(lib::L2CValue const&, lib::L2CValue const&, lib::L2CValue const&)"],
                    &ret_asm,
                    4);
            }
            // if (agent != "status_script")
            //     continue;

            uint64_t funcptr = resolved_syms[func + args];
            if (!funcptr) continue;
            
            printf_debug("Running %s(hash40(%s) => 0x%08x, ...)...\n", func.c_str(), hashstr.c_str(), x0);
            output = cluster.execute(funcptr, false, false, x0, x1, x2, x3);
            
            if (output)
            {
                printf("Got output %" PRIx64 " for %s(hash40(%s) => 0x%08" PRIx64 ", ...), mapping to %s\n", output, func.c_str(), hashstr.c_str(), x0, key.c_str());
                if (character == "common" && agent == "status_script") {
                    output = x0;
                }
                l2cagents[key] = output;
                l2cagents_rev[output] = key;
                
                // Special MSC stuff, they store funcs in a vtable
                // so we run function 9 to actually set everything
                if (agent == "status_script" && character != "common")
                {
                    uint64_t vtable_ptr = *(uint64_t*)(cluster.uc_ptr_to_real_ptr(output));
                    uint64_t* vtable = ((uint64_t*)(cluster.uc_ptr_to_real_ptr(vtable_ptr)));
                    uint64_t func = vtable[9];

                    cluster.clear_state();

                    cluster.execute(func, true, true, output);
                }
            }
        }
    }
    //logmask_set(LOGMASK_DEBUG | LOGMASK_INFO);
    //logmask_set(LOGMASK_VERBOSE);

    // Set up L2CAgent
    uint64_t luastate = cluster.heap_alloc(0x1000);
    
    for (int i = 0; i < 0x200; i += 8)
    {
        uint64_t class_alloc = cluster.heap_alloc(0x100);
        uint64_t vtable_alloc = cluster.heap_alloc(512 * sizeof(uint64_t));

        *(uint64_t*)(cluster.uc_ptr_to_real_ptr(luastate + i)) = class_alloc;
        *(uint64_t*)(cluster.uc_ptr_to_real_ptr(class_alloc)) = vtable_alloc;
        
        //printf("%llx %llx %llx\n", l2cagent, class_alloc, vtable_alloc);

        for (int j = 0; j < 512; j++)
        {
            uint64_t* out = (uint64_t*)cluster.uc_ptr_to_real_ptr(vtable_alloc + j * sizeof(uint64_t));
            uint64_t addr = IMPORTS + (imports_size + 0x8);
            imports_size += 0x8;

            snprintf(tmp, 255, "lua_State::off%XVtableFunc%u", i, j);
            
            /*if (i == 0x40 && j == 0x39)
            {
                printf("%s %llx\n", tmp, addr);
            }*/
            
            std::string name(tmp);
            
            unresolved_syms[name] = addr;
            unresolved_syms_rev[addr] = name;
            *out = addr;
            
            cluster.add_import_hook(addr);
        }
    }

    for (auto& pair : l2cagents)
    {
        uint64_t l2cagent = pair.second;
        L2CAgent* agent = (L2CAgent*)cluster.uc_ptr_to_real_ptr(l2cagent);
        agent->lua_state_agent = luastate;
        agent->lua_state_agentbase = luastate;
        // Set battle object, boma, lua_state if it hasn't been done already
        *(uint64_t*)(((uint64_t)agent) + 0x38) = x1;
        *(uint64_t*)(((uint64_t)agent) + 0x40) = x2;
        *(uint64_t*)(((uint64_t)agent)+ 0x48) = x3;
    }
    
    cluster.set_heap_fixed(true);
    for (auto& pair : function_hashes)
    {
        auto regpair = pair.first;
        uint64_t l2cagent = regpair.first;
        uint64_t funcptr = pair.second;
        uint64_t hash = regpair.second;
  
        //if (funcptr == 0x1000cb3b0)
        //if (funcptr == 0x1000cc6d0)
        //if (l2cagents_rev[l2cagent] == "wolf_ai_mode")
        {
            cluster_work(&cluster, character, std::string(argv[2]), l2cagent, funcptr, hash);
        }
    }
    
    while (clusters_active.load())
    {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(10ms);
    }
    
        
    cluster.clear_state();

    return 0;
}
