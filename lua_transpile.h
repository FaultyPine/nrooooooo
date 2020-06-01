#ifndef LUA_TRANSPILE_H
#define LUA_TRANSPILE_H

#include <stdint.h>
#include <map>
#include <set>

#include "l2c.h"
#include "lua_emitter.h"

class LuaTranspiler
{
private:
    std::string path;
    std::map<uint64_t, std::set<L2C_Token> > tokens;
    uint64_t func;
    
    LuaBytecodeEmitter emitter;

public:
    LuaTranspiler(std::string path, std::map<uint64_t, std::set<L2C_Token> > tokens, uint64_t func) : path(path), tokens(tokens), func(func), emitter(path) 
    {
        emitter.EmitLuacHeader();
        emitter.BeginFunction(0, 0, 0, 1, 2);
        luac_Constant cons = luac_Constant
        {
            type: luac_Integer,
            number: 0,
            integer: 10,//token.args[0],
            string: "",
        };
        emitter.constants.push_back(cons);
        // for (auto& pair: tokens) {
        //     break;
        //     for (auto token: pair.second) {
        //         std::string name = token.str;
        //         printf("%s\n", name.c_str());
        //         if (name == "lib::L2CAgent::clear_lua_stack()")
        //         {
        //         }
        //         else if (name == "app::sv_animcmd::is_excute(lua_State*)")
        //         {
        //             emitter.EmitOpGETTABUP(0, 0, 0);
        //             emitter.EmitOpGETTABLE(0, 0, 1);
        //             emitter.EmitOpTEST(0, 0);
        //         }
        //         // else if (name == "app::sv_animcmd::frame(lua_State*, float)")
        //         // {
        //         //     //token.args.push_back(args[0]);
        //         //     token.fargs.push_back(fargs[0]);

        //         //     inst->lua_stack.push_back(L2CValue(true));
        //         // }
        //         // else if (name == "lib::L2CAgent::pop_lua_stack(int)")
        //         // {
        //         //     token.args.push_back(args[1]);
                
        //         //     L2CValue* out = (L2CValue*)inst->uc_ptr_to_real_ptr(args[8]);
        //         //     L2CValue* iter = out;

        //         //     for (int i = 0; i < args[1]; i++)
        //         //     {
        //         //         if (!out) break;

        //         //         if (inst->lua_stack.size())
        //         //         {
        //         //             *iter = *(inst->lua_stack.end() - 1);
        //         //             inst->lua_stack.pop_back();
        //         //         }
        //         //         else
        //         //         {
        //         //             //printf_warn("Instance Id %u: Bad stack pop...\n", inst->get_id());
                            
        //         //             L2CValue empty();
        //         //             *iter = empty;
        //         //         }

        //         //         iter++;
        //         //     }
                    
        //         //     //inst->lua_active_vars[args[8]] = out;
        //         // }
        //         // else if (name == "lib::L2CAgent::push_lua_stack(lib::L2CValue const&)")
        //         // {
        //         //     L2CValue* val = (L2CValue*)inst->uc_ptr_to_real_ptr(args[1]);
                    
        //         //     if (val)
        //         //     {
        //         //         token.args.push_back(val->type);
        //         //         if (val->type != L2C_number)
        //         //         {
        //         //             token.args.push_back(val->raw);
        //         //         }
        //         //         else
        //         //         {
        //         //             token.fargs.push_back(val->as_number());
        //         //         }
                        
        //         //     }
        //         // }
        //         else if (name == "lib::L2CValue::L2CValue(int)")
        //         {
        //             emitter.constants.push_back(luac_Constant
        //                 {
        //                     type: luac_Integer,
        //                     number: 0,
        //                     integer: 10,//token.args[0],
        //                     string: "",
        //                 }
        //             );
        //             // emitter.EmitOpLOADK(0, emitter.constants.size()-1);
        //         }
        //         // else if (name == "lib::L2CValue::L2CValue(long)")
        //         // {
        //         //     L2CValue* var = (L2CValue*)inst->uc_ptr_to_real_ptr(args[0]);
        //         //     if (var)
        //         //         *var = L2CValue((long)args[1]);
        //         //     else
        //         //         printf_error("Instance Id %u: Bad L2CValue init, %" PRIx64 ", %" PRIx64 "\n", inst->get_id(), args[0], origin);
                
        //         //     token.args.push_back((long)args[1]);
        //         //     //add_token = false;
        //         //     //purge_markers(token.pc);
        //         // }
        //         // else if (name == "lib::L2CValue::L2CValue(unsigned int)"
        //         //         || name == "lib::L2CValue::L2CValue(unsigned long)")
        //         // {
        //         //     L2CValue* var = (L2CValue*)inst->uc_ptr_to_real_ptr(args[0]);
        //         //     if (var)
        //         //         *var = L2CValue(args[1]);
        //         //     else
        //         //         printf_error("Instance Id %u: Bad L2CValue init, %" PRIx64 ", %" PRIx64 "\n", inst->get_id(), args[0], origin);
                
        //         //     token.args.push_back(args[1]);
        //         //     //add_token = false;
        //         //     //purge_markers(token.pc);
        //         // }
        //         // else if (name == "lib::L2CValue::L2CValue(bool)")
        //         // {
        //         //     L2CValue* var = (L2CValue*)inst->uc_ptr_to_real_ptr(args[0]);
        //         //     if (var)
        //         //         *var = L2CValue((bool)args[1]);
        //         //     else
        //         //         printf_error("Instance Id %u: Bad L2CValue init, %" PRIx64 ", %" PRIx64 "\n", inst->get_id(), args[0], origin);
                
        //         //     token.args.push_back((int)args[1]);
        //         //     //add_token = false;
        //         //     //purge_markers(token.pc);
        //         // }
        //         // else if (name == "lib::L2CValue::L2CValue(phx::Hash40)")
        //         // {
        //         //     Hash40 hash = {args[1] & 0xFFFFFFFFFF};
        //         //     L2CValue* var = (L2CValue*)inst->uc_ptr_to_real_ptr(args[0]);
        //         //     if (var)
        //         //         *var = L2CValue(hash);
        //         //     else
        //         //         printf_error("Instance Id %u: Bad L2CValue init, %" PRIx64 ", %" PRIx64 "\n", inst->get_id(), args[0], origin);
                    
        //         //     token.args.push_back(hash.hash);
        //         //     //add_token = false;
        //         //     //purge_markers(token.pc);
        //         // }
        //         // else if (name == "lib::L2CValue::L2CValue(void*)")
        //         // {
        //         //     L2CValue* var = (L2CValue*)inst->uc_ptr_to_real_ptr(args[0]);
        //         //     if (var)
        //         //         *var = L2CValue((void*)args[1]);
        //         //     else
        //         //         printf_error("Instance Id %u: Bad L2CValue init, %" PRIx64 ", %" PRIx64 "\n", inst->get_id(), args[0], origin);
                    
        //         //     token.args.push_back(args[1]);
        //         //     //add_token = false;
        //         //     //purge_markers(token.pc);
        //         // }
        //         // else if (name == "lib::L2CValue::L2CValue(float)")
        //         // {
        //         //     L2CValue* var = (L2CValue*)inst->uc_ptr_to_real_ptr(args[0]);
        //         //     if (var)
        //         //         *var = L2CValue((float)fargs[0]);
        //         //     else
        //         //         printf_error("Instance Id %u: Bad L2CValue init, %" PRIx64 ", %" PRIx64 "\n", inst->get_id(), args[0], origin);
                    
        //         //     token.fargs.push_back(fargs[0]);
        //         //     //add_token = false;
        //         //     //purge_markers(token.pc);
        //         // }
        //         // else if (name == "lib::L2CValue::L2CValue(lib::L2CValue const&)")
        //         // {
        //         //     L2CValue* var = (L2CValue*)inst->uc_ptr_to_real_ptr(args[0]);
        //         //     L2CValue* var2 = (L2CValue*)inst->uc_ptr_to_real_ptr(args[1]);

        //         //     if (var && var2)
        //         //     {
        //         //         *var = L2CValue(var2);

        //         //         token.args.push_back(args[1]);
        //         //         token.args.push_back(var2->type);
                    
        //         //         if (var2->type == L2C_number)
        //         //             token.fargs.push_back(var2->as_number());
        //         //         else
        //         //             token.args.push_back(var2->raw);
        //         //     }
        //         //     else
        //         //         printf_error("Instance Id %u: Bad L2CValue init, %" PRIx64 ", %" PRIx64 ", %" PRIx64 "\n", inst->get_id(), args[0], args[1], origin);
                    

        //         // }
        //         // else if (name == "lib::L2CValue::as_number() const")
        //         // {
        //         //     L2CValue* var = (L2CValue*)inst->uc_ptr_to_real_ptr(args[0]);

        //         //     if (var)
        //         //     {
        //         //         fargs[0] = var->as_number();
        //         //         token.fargs.push_back(var->as_number());
        //         //     }
        //         //     else
        //         //         printf_error("Instance Id %u: Bad L2CValue access, %" PRIx64 ", %" PRIx64 "\n", inst->get_id(), args[0], origin);
        //         // }
        //         // else if (name == "lib::L2CValue::as_bool() const")
        //         // {
        //         //     L2CValue* var = (L2CValue*)inst->uc_ptr_to_real_ptr(args[0]);

        //         //     if (var)
        //         //     {
        //         //         args[0] = var->as_bool();
        //         //         token.args.push_back(var->as_bool());
        //         //     }
        //         //     else
        //         //         printf_error("Instance Id %u: Bad L2CValue access, %" PRIx64 ", %" PRIx64 "\n", inst->get_id(), args[0], origin);
        //         // }
        //         // else if (name == "lib::L2CValue::as_integer() const")
        //         // {
        //         //     L2CValue* var = (L2CValue*)inst->uc_ptr_to_real_ptr(args[0]);

        //         //     if (var)
        //         //     {
        //         //         args[0] = var->as_integer();
        //         //         token.args.push_back(var->as_integer());
        //         //     }
        //         //     else
        //         //         printf_error("Instance Id %u: Bad L2CValue access, %" PRIx64 ", %" PRIx64 "\n", inst->get_id(), args[0], origin);
        //         // }
        //         // else if (name == "lib::L2CValue::as_pointer() const")
        //         // {
        //         //     L2CValue* var = (L2CValue*)inst->uc_ptr_to_real_ptr(args[0]);

        //         //     if (var)
        //         //     {
        //         //         args[0] = var->raw;
        //         //         token.args.push_back(var->raw);
        //         //     }
        //         //     else
        //         //         printf_error("Instance Id %u: Bad L2CValue access, %" PRIx64 ", %" PRIx64 "\n", inst->get_id(), args[0], origin);
        //         // }
        //         // else if (name == "lib::L2CValue::as_table() const")
        //         // {
        //         //     L2CValue* var = (L2CValue*)inst->uc_ptr_to_real_ptr(args[0]);

        //         //     if (var)
        //         //     {
        //         //         args[0] = var->raw;
        //         //         token.args.push_back(var->raw);
        //         //     }
        //         //     else
        //         //         printf_error("Instance Id %u: Bad L2CValue access, %" PRIx64 ", %" PRIx64 "\n", inst->get_id(), args[0], origin);
        //         // }
        //         // else if (name == "lib::L2CValue::as_inner_function() const")
        //         // {
        //         //     L2CValue* var = (L2CValue*)inst->uc_ptr_to_real_ptr(args[0]);

        //         //     if (var)
        //         //     {
        //         //         args[0] = var->raw;
        //         //         token.args.push_back(var->raw);
        //         //     }
        //         //     else
        //         //         printf_error("Instance Id %u: Bad L2CValue access, %" PRIx64 ", %" PRIx64 "\n", inst->get_id(), args[0], origin);
        //         // }
        //         // else if (name == "lib::L2CValue::as_hash() const")
        //         // {
        //         //     L2CValue* var = (L2CValue*)inst->uc_ptr_to_real_ptr(args[0]);

        //         //     if (var)
        //         //     {
        //         //         args[0] = var->as_hash();
        //         //         token.args.push_back(var->as_hash());
        //         //     }
        //         //     else
        //         //         printf_error("Instance Id %u: Bad L2CValue access, %" PRIx64 ", %" PRIx64 "\n", inst->get_id(), args[0], origin);
        //         // }
        //         // else if (name == "lib::L2CValue::as_string() const")
        //         // {
        //         //     L2CValue* var = (L2CValue*)inst->uc_ptr_to_real_ptr(args[0]);

        //         //     if (var)
        //         //     {
        //         //         args[0] = var->raw;
        //         //         token.args.push_back(var->raw);
        //         //     }
        //         //     else
        //         //         printf_error("Instance Id %u: Bad L2CValue access, %" PRIx64 ", %" PRIx64 "\n", inst->get_id(), args[0], origin);
        //         // }
        //         // else if (name == "lib::L2CValue::~L2CValue()")
        //         // {
        //         //     //inst->lua_active_vars[args[0]] = nullptr;
        //         //     //add_token = false;
        //         //     //purge_markers(token.pc);
        //         // }
        //         // else if (name == "lib::L2CValue::operator[](phx::Hash40) const")
        //         // {
        //         //     if (!hash_cheat[args[1]])
        //         //     {
        //         //         hash_cheat[args[1]] = inst->heap_alloc(0x10);
        //         //     }

        //         //     uint64_t l2cval = hash_cheat[args[1]];
        //         //     hash_cheat_rev[l2cval] = args[1];

        //         //     printf_verbose("Hash cheating!! %llx\n", l2cval);
                    
        //         //     args[0] = l2cval;
                    
        //         //     token.args.push_back(args[1]);
        //         // }
        //         // else if (name == "lib::L2CValue::operator[](int) const")
        //         // {
        //         //     //TODO impl
        //         //     //token.args.push_back(args[0]);
        //         //     token.args.push_back(args[1]);
        //         // }
        //         // else if (name == "lib::L2CValue::operator=(lib::L2CValue const&)")
        //         // {
        //         //     L2CValue* out = (L2CValue*)inst->uc_ptr_to_real_ptr(args[0]);
        //         //     L2CValue* in = (L2CValue*)inst->uc_ptr_to_real_ptr(args[1]);
                    
        //         //     if (in && out)
        //         //     {
        //         //         //TODO operator= destruction
        //         //         *out = *in;
                        
        //         //         if (hash_cheat_rev[args[0]])
        //         //         {
        //         //             printf_verbose("Hash cheating! %llx => %llx\n", hash_cheat_rev[args[0]], in->raw);
        //         //             function_hashes[std::pair<uint64_t, uint64_t>(hash_cheat_ptr, hash_cheat_rev[args[0]])] = in->raw;
        //         //         }
        //         //     }
        //         //     else
        //         //     {
        //         //         printf_error("Instance Id %u: Bad L2CValue assignment @ " PRIx64 "!\n", inst->get_id(), origin);
        //         //     }
        //         // }
        //         // else if (name == "lib::L2CValue::operator bool() const"
        //         //         || name == "lib::L2CValue::operator==(lib::L2CValue const&) const"
        //         //         || name == "lib::L2CValue::operator<=(lib::L2CValue const&) const"
        //         //         || name == "lib::L2CValue::operator<(lib::L2CValue const&) const")
        //         // {
        //         //     //TODO basic emu comparisons
        //         //     if (inst->is_basic_emu())
        //         //     {
        //         //         L2CValue* in = (L2CValue*)inst->uc_ptr_to_real_ptr(args[0]);
        //         //         if (in)
        //         //             args[0] = in->as_bool();
        //         //         else
        //         //             args[0] = 0;
        //         //     }
        //         //     else
        //         //     {
        //         //         if (add_token)
        //         //             cluster->add_subreplace_token(inst, origin_block, token);
        //         //         add_token = false;

        //         //         inst->regs_cur.x0 = 1;
        //         //         inst->regs_flush();
        //         //         inst->fork_inst();

        //         //         inst->regs_cur.x0 = 0;
        //         //         args[0] = 0;
        //         //     }
        //         // }
        //     }
        // }
        emitter.EmitOpRETURN(0, 1);
        emitter.EmitUpvalue(1, 0); // ENV
    }
    ~LuaTranspiler() 
    {
        emitter.FinalizeFunction();
    }
    
    
};

#endif // LUA_TRANSPILE_H
