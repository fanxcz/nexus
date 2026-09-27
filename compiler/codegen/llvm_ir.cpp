#include "codegen/llvm_ir.hpp"
#include <sstream>
#include <unordered_map>
#include <stdexcept>
#include <vector>
#include <cstdio>

namespace nexus {
class CG {
public:
    std::ostringstream module, functionsIR, body;
    int reg=0, label=0;
    std::unordered_map<std::string,std::string> vars;
    std::unordered_map<std::string,Type> types;
    std::unordered_map<std::string,Function*> functions;
    std::vector<std::string> globals;
    bool terminated=false;

    std::string next(){return "%"+std::to_string(++reg);}
    std::string lab(const std::string&p){return p+std::to_string(++label);}
    void emit(const std::string&s){body<<s<<"\n";}
    void emitModule(const std::string&s){module<<s<<"\n";}
    static std::string llvmType(Type t){switch(t.kind){case TypeKind::I64:return "i64";case TypeKind::Bool:return "i1";case TypeKind::String:return "ptr";case TypeKind::Void:return "void";default:return "i64";}}
    std::string esc(const std::string&s){
        std::ostringstream q;
        for(unsigned char c:s){
            switch(c){case '\\':q<<"\\5C";break;case '"':q<<"\\22";break;case '\n':q<<"\\0A";break;case '\r':q<<"\\0D";break;case '\t':q<<"\\09";break;default: if(c<32){char b[5];std::snprintf(b,sizeof b,"\\%02X",c);q<<b;}else q<<c;}
        }
        return q.str();
    }
    Type typeOf(const Expr*e){
        if(dynamic_cast<const IntExpr*>(e)) return Type::i64();
        if(dynamic_cast<const BoolExpr*>(e)) return Type::boolean();
        if(dynamic_cast<const StringExpr*>(e)) return Type::string();
        if(auto*v=dynamic_cast<const VarExpr*>(e)) return types.at(v->name);
        if(auto*u=dynamic_cast<const UnaryExpr*>(e)) return typeOf(u->rhs.get());
        if(auto*b=dynamic_cast<const BinaryExpr*>(e)){
            switch(b->op){case TokenKind::EqualEqual:case TokenKind::BangEqual:case TokenKind::Less:case TokenKind::LessEqual:case TokenKind::Greater:case TokenKind::GreaterEqual:case TokenKind::AndAnd:case TokenKind::OrOr:return Type::boolean();default:return Type::i64();}
        }
        if(auto*c=dynamic_cast<const CallExpr*>(e)){if(c->callee=="print") return Type::void_(); return functions.at(c->callee)->ret;}
        return Type::i64();
    }
    std::string stringValue(const std::string&s){
        std::string g="@.str"+std::to_string(globals.size());
        size_t n=s.size()+1;
        std::ostringstream x; x<<g<<" = private unnamed_addr constant ["<<n<<" x i8] c\""<<esc(s)<<"\\00\", align 1";
        globals.push_back(x.str());
        std::string p=next();
        emit(p+" = getelementptr inbounds ["+std::to_string(n)+" x i8], ptr "+g+", i64 0, i64 0");
        return p;
    }
    std::string loadVar(const std::string&name){auto r=next();emit(r+" = load "+llvmType(types.at(name))+", ptr "+vars.at(name));return r;}
    std::string expr(const Expr*e){
        if(auto*x=dynamic_cast<const IntExpr*>(e)) return std::to_string(x->value);
        if(auto*x=dynamic_cast<const BoolExpr*>(e)) return x->value?"true":"false";
        if(auto*x=dynamic_cast<const StringExpr*>(e)) return stringValue(x->value);
        if(auto*x=dynamic_cast<const VarExpr*>(e)) return loadVar(x->name);
        if(auto*u=dynamic_cast<const UnaryExpr*>(e)){
            auto a=expr(u->rhs.get()); auto r=next();
            if(u->op==TokenKind::Minus){emit(r+" = sub i64 0, "+a);return r;}
            emit(r+" = xor i1 "+a+", true"); return r;
        }
        if(auto*b=dynamic_cast<const BinaryExpr*>(e)){
            auto l=expr(b->lhs.get()), rvalue=expr(b->rhs.get()), r=next();
            switch(b->op){
                case TokenKind::Plus:emit(r+" = add i64 "+l+", "+rvalue);break;
                case TokenKind::Minus:emit(r+" = sub i64 "+l+", "+rvalue);break;
                case TokenKind::Star:emit(r+" = mul i64 "+l+", "+rvalue);break;
                case TokenKind::Slash:emit(r+" = sdiv i64 "+l+", "+rvalue);break;
                case TokenKind::Percent:emit(r+" = srem i64 "+l+", "+rvalue);break;
                case TokenKind::EqualEqual:emit(r+" = icmp eq i64 "+l+", "+rvalue);break;
                case TokenKind::BangEqual:emit(r+" = icmp ne i64 "+l+", "+rvalue);break;
                case TokenKind::Less:emit(r+" = icmp slt i64 "+l+", "+rvalue);break;
                case TokenKind::LessEqual:emit(r+" = icmp sle i64 "+l+", "+rvalue);break;
                case TokenKind::Greater:emit(r+" = icmp sgt i64 "+l+", "+rvalue);break;
                case TokenKind::GreaterEqual:emit(r+" = icmp sge i64 "+l+", "+rvalue);break;
                case TokenKind::AndAnd:emit(r+" = and i1 "+l+", "+rvalue);break;
                case TokenKind::OrOr:emit(r+" = or i1 "+l+", "+rvalue);break;
                default:throw std::runtime_error("unsupported binary operator in codegen");
            }
            return r;
        }
        if(auto*c=dynamic_cast<const CallExpr*>(e)){
            if(c->callee=="print"){
                auto av=expr(c->args[0].get()); auto t=typeOf(c->args[0].get());
                if(t.kind==TypeKind::I64) emit("call void @nexus_print_i64(i64 "+av+")");
                else if(t.kind==TypeKind::Bool) emit("call void @nexus_print_bool(i1 "+av+")");
                else if(t.kind==TypeKind::String) emit("call void @nexus_print_str(ptr "+av+")");
                return "0";
            }
            auto*fn=functions.at(c->callee); std::ostringstream call; call<<"call "<<llvmType(fn->ret)<<" @"<<fn->name<<"(";
            for(size_t i=0;i<c->args.size();++i){if(i)call<<", ";auto av=expr(c->args[i].get());call<<llvmType(fn->params[i].type)<<" "<<av;}
            call<<")";
            if(fn->ret.kind==TypeKind::Void){emit(call.str());return "0";}
            auto r=next();emit(r+" = "+call.str());return r;
        }
        throw std::runtime_error("unsupported expression in codegen");
    }
    bool stmt(const Stmt*s,Type ret){
        if(terminated) return true;
        if(auto*l=dynamic_cast<const LetStmt*>(s)){
            auto ptr=next(); vars[l->name]=ptr; types[l->name]=l->type; emit(ptr+" = alloca "+llvmType(l->type)); auto v=expr(l->init.get()); emit("store "+llvmType(l->type)+" "+v+", ptr "+ptr); return false;
        }
        if(auto*a=dynamic_cast<const AssignStmt*>(s)){
            auto v=expr(a->value.get()); emit("store "+llvmType(types.at(a->name))+" "+v+", ptr "+vars.at(a->name)); return false;
        }
        if(auto*e=dynamic_cast<const ExprStmt*>(s)){expr(e->expr.get());return false;}
        if(auto*r=dynamic_cast<const ReturnStmt*>(s)){if(r->expr){auto v=expr(r->expr.get());emit("ret "+llvmType(ret)+" "+v);}else emit("ret void");terminated=true;return true;}
        if(auto*i=dynamic_cast<const IfStmt*>(s)){
            auto c=expr(i->cond.get()); auto lt=lab("if.then"), lf=lab("if.else"), le=lab("if.end"); emit("br i1 "+c+", label %"+lt+", label %"+lf);
            emit(lt+":"); terminated=false; for(auto&x:i->thenBlock->statements)stmt(x.get(),ret); bool thenTerm=terminated;
            if(!thenTerm) emit("br label %"+le);
            emit(lf+":"); terminated=false; bool elseTerm=false; if(i->elseBlock){for(auto&x:i->elseBlock->statements)stmt(x.get(),ret);elseTerm=terminated;}
            if(!elseTerm) emit("br label %"+le);
            if(thenTerm&&elseTerm){terminated=true;return true;} emit(le+":"); terminated=false; return false;
        }
        if(auto*w=dynamic_cast<const WhileStmt*>(s)){
            auto lc=lab("while.cond"), lb=lab("while.body"), le=lab("while.end"); emit("br label %"+lc); emit(lc+":"); auto c=expr(w->cond.get()); emit("br i1 "+c+", label %"+lb+", label %"+le);
            emit(lb+":"); terminated=false; for(auto&x:w->body->statements)stmt(x.get(),ret); if(!terminated)emit("br label %"+lc);
            emit(le+":"); terminated=false; return false;
        }
        return false;
    }
    std::string generate(const Program&p){
        for(auto&f:const_cast<Program&>(p).functions) functions[f.name]=&f;
        emitModule("; Nexus LLVM IR v0.1.0"); emitModule("target triple = \"x86_64-unknown-linux-gnu\"");
        emitModule("declare void @nexus_print_i64(i64)"); emitModule("declare void @nexus_print_bool(i1)"); emitModule("declare void @nexus_print_str(ptr)");
        for(auto&f:p.functions){
            vars.clear();types.clear();reg=0;label=0;terminated=false; body.str("");body.clear();
            Type codegenRet=f.ret; if(f.name=="main" && codegenRet.kind==TypeKind::Void) codegenRet=Type::i64();
            std::ostringstream sig; sig<<"define "<<llvmType(codegenRet)<<" @"<<f.name<<"(";
            for(size_t i=0;i<f.params.size();++i){if(i)sig<<", ";sig<<llvmType(f.params[i].type)<<" %arg"<<i;} sig<<") {";
            emit(sig.str());
            emit("entry:");
            for(size_t i=0;i<f.params.size();++i){auto ptr=next();vars[f.params[i].name]=ptr;types[f.params[i].name]=f.params[i].type;emit(ptr+" = alloca "+llvmType(f.params[i].type));emit("store "+llvmType(f.params[i].type)+" %arg"+std::to_string(i)+", ptr "+ptr);}
            for(auto&s:f.body->statements)stmt(s.get(),codegenRet);
            if(!terminated){if(codegenRet.kind==TypeKind::Void)emit("ret void");else if(f.name=="main" && codegenRet.kind==TypeKind::I64)emit("ret i64 0");else throw std::runtime_error("function may fall through without return: "+f.name);}
            emit("}"); functionsIR<<body.str();
        }
        std::ostringstream result; result<<module.str(); for(auto&g:globals) result<<g<<"\n"; result<<functionsIR.str(); return result.str();
    }
};
std::string LLVMIRGenerator::generate(const Program&p){return CG{}.generate(p);} 
}
