#include "semantic/semantic.hpp"
#include <stdexcept>
namespace nexus {
const StructDecl& SemanticAnalyzer::getStruct(const std::string& n) const { auto it=structs_.find(n); if(it==structs_.end()) throw std::runtime_error("unknown struct: "+n); return *it->second; }
static const StructField* fieldOf(const StructDecl&s,const std::string& n){for(auto&f:s.fields)if(f.name==n)return &f;return nullptr;}
void SemanticAnalyzer::analyze(Program& p){
    funcs_.clear(); structs_.clear();
    for(auto& s:p.structs){if(structs_.contains(s.name))throw std::runtime_error("duplicate struct: "+s.name);structs_[s.name]=&s;}
    for(auto& e:p.enums){if(structs_.contains(e.name))throw std::runtime_error("duplicate type: "+e.name);}
    for(auto& f:p.functions){if(funcs_.contains(f.name))throw std::runtime_error("duplicate function: "+f.name);funcs_[f.name]=&f;}
    for(auto& s:p.structs)for(auto&f:s.fields){if(f.type.kind==TypeKind::Struct && !structs_.contains(f.type.name))throw std::runtime_error("unknown field type: "+f.type.name);}
    for(auto& f:p.functions){if(f.external)continue;std::unordered_map<std::string,Symbol> env;for(auto&a:f.params){if(env.contains(a.name))throw std::runtime_error("duplicate parameter: "+a.name);env[a.name]={a.type,true};} if(f.body)block(*f.body,env,f.ret,false);}
}
Type SemanticAnalyzer::expr(Expr* e,std::unordered_map<std::string,Symbol>& env){
    if(dynamic_cast<IntExpr*>(e))return Type::i64(); if(dynamic_cast<FloatExpr*>(e))return Type::f64(); if(dynamic_cast<BoolExpr*>(e))return Type::boolean(); if(dynamic_cast<StringExpr*>(e))return Type::string();
    if(auto*v=dynamic_cast<VarExpr*>(e)){auto it=env.find(v->name);if(it==env.end())throw std::runtime_error("unknown variable: "+v->name);return it->second.type;}
    if(auto*u=dynamic_cast<UnaryExpr*>(e)){auto t=expr(u->rhs.get(),env);if(u->op==TokenKind::Minus&&(t.kind!=TypeKind::I64&&t.kind!=TypeKind::F64))throw std::runtime_error("unary '-' requires number");if(u->op==TokenKind::Bang&&t.kind!=TypeKind::Bool)throw std::runtime_error("'!' requires bool");if(u->op==TokenKind::Ampersand){if(auto*v=dynamic_cast<VarExpr*>(u->rhs.get())){auto it=env.find(v->name);if(it==env.end())throw std::runtime_error("unknown variable: "+v->name);}else if(!dynamic_cast<MemberExpr*>(u->rhs.get()))throw std::runtime_error("'&' requires a variable or field");return Type::ptr(t);}if(u->op==TokenKind::Star){if(t.kind!=TypeKind::Pointer||!t.pointee)throw std::runtime_error("'*' requires pointer");return *t.pointee;}return t;}
    if(auto*b=dynamic_cast<BinaryExpr*>(e)){
        auto l=expr(b->lhs.get(),env), r=expr(b->rhs.get(),env);
        if(!sameType(l,r)) throw std::runtime_error("binary operands have different types");
        switch(b->op){
            case TokenKind::Plus:
                if(l.kind==TypeKind::String || l.kind==TypeKind::I64 || l.kind==TypeKind::F64) return l;
                throw std::runtime_error("operator + requires numbers or strings");
            case TokenKind::Minus:
            case TokenKind::Star:
            case TokenKind::Slash:
            case TokenKind::Percent:
                if(l.kind!=TypeKind::I64 && l.kind!=TypeKind::F64) throw std::runtime_error("arithmetic requires numeric types");
                return l;
            case TokenKind::EqualEqual:
            case TokenKind::BangEqual:
                if(l.kind==TypeKind::String || l.kind==TypeKind::I64 || l.kind==TypeKind::F64 || l.kind==TypeKind::Bool) return Type::boolean();
                throw std::runtime_error("equality is unsupported for this type");
            case TokenKind::Less:
            case TokenKind::LessEqual:
            case TokenKind::Greater:
            case TokenKind::GreaterEqual:
                if(l.kind==TypeKind::String) throw std::runtime_error("string ordering is not supported; use == or !=");
                if(l.kind==TypeKind::I64 || l.kind==TypeKind::F64) return Type::boolean();
                throw std::runtime_error("comparison requires numeric types");
            case TokenKind::AndAnd:
            case TokenKind::OrOr:
                if(l.kind!=TypeKind::Bool) throw std::runtime_error("logical operators require bool");
                return Type::boolean();
            default:
                break;
        }
    }
    if(auto*c=dynamic_cast<CallExpr*>(e)){if(c->callee=="print"){if(c->args.size()!=1)throw std::runtime_error("print expects one argument");auto t=expr(c->args[0].get(),env);if(t.kind!=TypeKind::I64&&t.kind!=TypeKind::F64&&t.kind!=TypeKind::Bool&&t.kind!=TypeKind::String)throw std::runtime_error("print: unsupported type");return Type::void_();}auto it=funcs_.find(c->callee);if(it==funcs_.end())throw std::runtime_error("unknown function: "+c->callee);auto*f=it->second;if(f->params.size()!=c->args.size())throw std::runtime_error("wrong argument count for "+c->callee);for(size_t i=0;i<c->args.size();++i)if(!sameType(expr(c->args[i].get(),env),f->params[i].type))throw std::runtime_error("argument type mismatch in "+c->callee);return f->ret;}
    if(auto*m=dynamic_cast<MemberExpr*>(e)){auto t=expr(m->object.get(),env);if(t.kind!=TypeKind::Struct)throw std::runtime_error("member access requires struct");auto& s=getStruct(t.name);auto*f=fieldOf(s,m->member);if(!f)throw std::runtime_error("unknown field '"+m->member+"' on "+t.name);return f->type;}
    if(auto*si=dynamic_cast<StructInitExpr*>(e)){auto&s=getStruct(si->typeName);if(si->fields.size()!=s.fields.size())throw std::runtime_error("wrong number of fields for "+si->typeName);for(auto&[name,x]:si->fields){auto*f=fieldOf(s,name);if(!f)throw std::runtime_error("unknown field '"+name+"'");if(!sameType(expr(x.get(),env),f->type))throw std::runtime_error("field type mismatch for "+name);}return Type::named(si->typeName);}
    throw std::runtime_error("unsupported expression");
}
void SemanticAnalyzer::block(Block& b,std::unordered_map<std::string,Symbol>& env,Type ret,bool inLoop){
    for(auto&s:b.statements){
        if(auto*l=dynamic_cast<LetStmt*>(s.get())){if(env.contains(l->name))throw std::runtime_error("duplicate variable: "+l->name);auto t=expr(l->init.get(),env);if(l->type.kind!=TypeKind::Unknown&&!sameType(l->type,t))throw std::runtime_error("type mismatch for "+l->name);l->type=t;env[l->name]={t,l->mut};}
        else if(auto*a=dynamic_cast<AssignStmt*>(s.get())){auto it=env.find(a->name);if(it==env.end())throw std::runtime_error("unknown variable: "+a->name);if(!it->second.mut)throw std::runtime_error("cannot assign to immutable variable: "+a->name);if(!sameType(expr(a->value.get(),env),it->second.type))throw std::runtime_error("assignment type mismatch for "+a->name);}
        else if(auto*ma=dynamic_cast<MemberAssignStmt*>(s.get())){auto t=expr(ma->object.get(),env);if(t.kind!=TypeKind::Struct)throw std::runtime_error("member assignment requires struct");if(auto*ov=dynamic_cast<VarExpr*>(ma->object.get())){auto it=env.find(ov->name);if(it!=env.end()&&!it->second.mut)throw std::runtime_error("cannot modify field through immutable variable: "+ov->name);}auto& st=getStruct(t.name);auto*f=fieldOf(st,ma->member);if(!f)throw std::runtime_error("unknown field: "+ma->member);if(!sameType(expr(ma->value.get(),env),f->type))throw std::runtime_error("field assignment type mismatch");}
        else if(auto*e=dynamic_cast<ExprStmt*>(s.get()))expr(e->expr.get(),env);
        else if(auto*r=dynamic_cast<ReturnStmt*>(s.get())){auto t=r->expr?expr(r->expr.get(),env):Type::void_();if(!sameType(t,ret))throw std::runtime_error("return type mismatch");}
        else if(auto*ifs=dynamic_cast<IfStmt*>(s.get())){if(expr(ifs->cond.get(),env).kind!=TypeKind::Bool)throw std::runtime_error("if condition must be bool");auto a=env;block(*ifs->thenBlock,a,ret,inLoop);if(ifs->elseBlock){auto c=env;block(*ifs->elseBlock,c,ret,inLoop);}}
        else if(auto*w=dynamic_cast<WhileStmt*>(s.get())){if(expr(w->cond.get(),env).kind!=TypeKind::Bool)throw std::runtime_error("while condition must be bool");auto a=env;block(*w->body,a,ret,true);}
        else if(dynamic_cast<BreakStmt*>(s.get())||dynamic_cast<ContinueStmt*>(s.get())){if(!inLoop)throw std::runtime_error("break/continue used outside loop");}
    }
}
}
