/*
 * Created by v1tr10l7 on 04.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Library/Logger.hpp>
#include <Prism/String/String.hpp>

namespace ACPI
{
    enum class ObjectType 
    {
        eScope = 0x00,
        eDevice = 0x01,
        eMethod = 0x02,
        eField = 0x03,
        eBuffer = 0x04,
        eString = 0x05,
        eInteger = 0x06,
        ePackage = 0x07,
        eMutex = 0x08,
        eOperationRegion = 0x09,
        eProcessor = 0x0a,
        eThermalZone = 0x0b,
        eAlias = 0x0c,
        eName = 0x0d,
        eWhile = 0x0e,
        eUndefined = 0x0f,
    };
    class Object 
    {
      public:
        Object(ObjectType type)
          : m_Type(type)
        {

        }

      protected:
        ObjectType m_Type = ObjectType::eUndefined;  
    };

    class IntegerObject : public Object 
    {
      public:
        IntegerObject(usize value)
            : Object(ObjectType::eInteger), m_Value(value)
        {

        }

        constexpr usize Value() const { return m_Value; }
        
      private:
        usize m_Value = 0;
    };

    enum class ExpressionType { eConstant, eNameRef, eBinaryOp, eCall };
    enum class StatementType { eStore, eWhile, eExpressionStatement, eCallStatement };
    enum class BinaryOpcode { eAdd, eSubtract, eLess };

    struct Expression 
    {
        ExpressionType Type;
        virtual ~Expression() = default;
        virtual void Dump(int indent = 0) const = 0;
    };


    struct ConstantExpression : Expression 
    {
        u64 Value;
        ConstantExpression(u64 val) : Value(val) { Type = ExpressionType::eConstant; }

        void Dump(int indent = 0) const override 
        {
            for (int i = 0; i < indent; i++)
                LogMessage(" ");
            LogMessage("Const({})\n", Value);
        }
    };

    struct NameRefExpr : Expression 
    {
        String Name;
        NameRefExpr(String name) : Name(Move(name)) { Type = ExpressionType::eNameRef; }

        void Dump(int indent = 0) const override 
        {
            for (int i = 0; i < indent; i++)
                LogMessage(" ");
            LogMessage("Ref({})\n", Name);
        }
    };

    struct BinaryExpression : Expression 
    {
        BinaryOpcode Op;
        Expression* Left;
        Expression* Right;
        BinaryExpression(BinaryOpcode op, Expression* l, Expression* r)
            : Op(op), Left(l), Right(r) { Type = ExpressionType::eBinaryOp; }

        void Dump(int indent = 0) const override 
        {
            for (int i = 0; i < indent; i++)
                LogMessage(" ");

            char sign = Op == BinaryOpcode::eSubtract ? '-' : '<';
            LogMessage("BinOp({})\n", sign);
            Left->Dump(indent + 2);
            Right->Dump(indent + 2);
        }
    };

    struct CallExpression : Expression 
    {
        String Callee;
        Vector<Expression*> Args;
        CallExpression(String callee) : Callee(Move(callee)) { Type = ExpressionType::eCall; }

        void Dump(int indent = 0) const override 
        {
            for (int i = 0; i < indent; i++)
                LogMessage(" ");
            LogMessage("Call({})\n", Callee);
            for (const auto& arg : Args)
                arg->Dump(indent + 2);
        }
    };

    class Statement 
    {
      public:
        StatementType Type;
        virtual ~Statement() = default;

        virtual void Dump(int indent = 0) const = 0;
    };
    struct StoreStatement : Statement 
    {
        Expression* Src;
        Expression* Dst;
        StoreStatement(Expression* src, Expression* dst) : Src(src), Dst(dst) { Type = StatementType::eStore; }

        void Dump(int indent = 0) const override 
        {
            for (int i = 0; i < indent; i++)
                LogMessage(" ");
            LogMessage("StoreStmt\n");
            Src->Dump(indent + 2);
            Dst->Dump(indent + 2);
        }
    };

    struct ExpressionStatement : Statement 
    {
        Expression* Expr;
        ExpressionStatement(Expression* e) : Expr(e) { Type = StatementType::eExpressionStatement; }

        void Dump(int indent = 0) const override 
        {
            for (int i = 0; i < indent; i++)
                LogMessage(" ");
            LogMessage("ExpressionStmt\n");
            Expr->Dump(indent + 2);
        }
    };

    struct WhileStatement : Statement 
    {
        Expression* Cond;
        Vector<Statement*> Body;
        WhileStatement(Expression* cond) : Cond(cond) 
        { 
            Type = StatementType::eWhile; 
        }

        void Dump(int indent = 0) const override 
        {
            for (int i = 0; i < indent; i++)
                LogMessage(" ");
            LogMessage("WhileStmt\n");
            Cond->Dump(indent + 2);
            for (const auto& stmt : Body)
                stmt->Dump(indent + 4);
        }
    };
    struct CallStatement : Statement 
    {
        CallExpression* Expression;
        CallStatement(CallExpression* c) : Expression(c) { Type = StatementType::eCallStatement; }
        void Dump(int indent = 0) const override 
        {
            for (int i = 0; i < indent; i++)
                LogMessage(" ");
            LogMessage("CallStmt\n");
            
            Expression->Dump(indent + 2);
        } 
    };

    class MethodObject : public Object 
    {
      public:
        MethodObject()
            : Object(ObjectType::eMethod)
        {

        }

        void Dump() const 
        {
            LogMessage("Method: {}, Args: {}\n", m_Name, int(m_ArgumentCount));
            for (const auto& stmt : m_Statements)
                stmt->Dump(2);
        }

        StringView m_Name = "";
        [[maybe_unused]] u8 m_ArgumentCount = 0;
        Vector<Statement*> m_Statements;
    };

    inline Expression* Const(u64 val) { return new ConstantExpression(val); }
    inline Expression* Ref(const String& name) { return new NameRefExpr(name); }
    inline Expression* BinOp(BinaryOpcode op, Expression* l, Expression* r) 
    {
        return new BinaryExpression(op, l, r);
    }
    inline Expression* Call(const String& callee, std::initializer_list<Expression*> args) 
    {
        auto call = new CallExpression(callee);
        for (auto* arg : args)
            call->Args.EmplaceBack(arg);
        return call;
    }
};
