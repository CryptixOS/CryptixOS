/*
 * Created by v1tr10l7 on 28.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Boot/CommandLine.hpp>
#include <Firmware/ACPI/Interpreter/Interpreter.hpp>

#include <Prism/Containers/Array.hpp>
#include <Prism/Containers/Deque.hpp>
#include <Prism/String/String.hpp>
#include <Prism/Utility/Optional.hpp>

namespace ACPI::Interpreter
{
    static NameSpace*       s_RootNameSpace = nullptr;
    static Vector<CodeBlock> s_ScopesToParse;
    static ExecutionContext s_Context;
    static CodeBlock        s_CurrentBlock;

    inline u8             PeekNextByte()
    {
        auto& stream = s_Context.Stream;

        u8  value  = 0;
        stream >> value;

        stream.Seek(stream.Offset() - 1);
        return value;
    }

    template <UnsignedIntegral T>
    inline T ReadNext()
    {
        T value = 0;
        s_Context.Stream >> value;

        return value;
    }
    inline OpCode GetNextOp()
    {
        u8 high;
        s_Context.Stream >> high;

        if (high == 0x5b)
        {
            u8 low;
            s_Context.Stream >> low;

            return static_cast<OpCode>((high << 8) | low);
        }

        return static_cast<OpCode>(high);
    }

    u32 DecodePkgLength()
    {
        u8  leadByte  = ReadNext<u8>();

        u8  byteCount = (leadByte >> 6) & 0x03;
        u32 length    = leadByte & 0x3f;

        for (usize i = 0; i < byteCount; ++i)
        {
            u8 nextByte = ReadNext<u8>();
            length |= static_cast<u32>(nextByte) << (4 + (i * 8));
        }
        return length;
    }

    constexpr bool IsNameSegment(u8 op)
    {
        return op == 0x2e || op == 0x2f || op == '\\' || op == '^'
            || (op >= 'A' && op <= 'Z') || op == 0x00 || op == '_';
    }
    String DecodeNameSegment()
    {
        char segments[5] = {0};
        for (usize i = 0; i < 4; ++i) s_Context.Stream >> segments[i];
        return String(segments);
    }
    String DecodeName()
    {
        String name;

        if (PeekNextByte() == '\\') name += ReadNext<u8>();
        while (PeekNextByte() == '^') name += ReadNext<u8>();
        constexpr usize DUAL_PREFIX  = 0x2e;
        constexpr usize MULTI_PREFIX = 0x2f;

        u8              segmentCount = 0;
        if (PeekNextByte() == '\0')
        {
            segmentCount = 0;
            ReadNext<u8>();
        }
        else if (PeekNextByte() == DUAL_PREFIX)
        {
            segmentCount = 2;
            ReadNext<u8>();
        }
        else if (PeekNextByte() == MULTI_PREFIX)
        {
            ReadNext<u8>();
            segmentCount = ReadNext<u8>();
        }
        else segmentCount = 1;

        while (segmentCount-- > 0)
        {
            auto segment = DecodeNameSegment();
            name += segment;
        }
        return name;
    }

    Expression* ParseSuperName()
    {
        auto opcode = GetNextOp();
        switch (opcode)
        {
            case OpCode::eLocal0... OpCode::eLocal7:
            case OpCode::eArg0... OpCode::eArg6:
            {
                StringView argName = magic_enum::enum_name(opcode).data() + 1;
                return new NameRefExpr(argName);
            }

            default:
                if (IsNameSegment(ToUnderlying(opcode)))
                {
                    if (ToUnderlying(opcode) != 0)
                        s_Context.Stream.Seek(s_Context.Stream.Offset() - 1);
                    return new NameRefExpr(DecodeName());
                }

                LogError("Unknown AML opcode: {:#x}", ToUnderlying(opcode));
                return nullptr;
        }
    }
    Expression* ParseTermArg()
    {
        u8   op     = ReadNext<u8>();
        OpCode opcode = static_cast<OpCode>(op);

        switch (opcode)
        {
            case OpCode::eLocal0... OpCode::eLocal7:
            case OpCode::eArg0... OpCode::eArg6:
            {
                StringView argName = magic_enum::enum_name(opcode).data() + 1;
                return new NameRefExpr(argName);
            }
            case OpCode::eZero: return new ConstantExpression(0);
            case OpCode::eOne: return new ConstantExpression(1);
            case OpCode::eOnes: return new ConstantExpression(~0ull);
            case OpCode::eBytePrefix:
                return new ConstantExpression(ReadNext<u8>());
            case OpCode::eWordPrefix:
                return new ConstantExpression(ReadNext<u16>());
            case OpCode::eDWordPrefix:
                return new ConstantExpression(ReadNext<u32>());
            case OpCode::eQWordPrefix:
                return new ConstantExpression(ReadNext<u64>());
            case OpCode::eSizeOf:
            {
                auto arg = ParseTermArg();
                return Call("SizeOf", {arg});
            }
            case OpCode::eIndex:
            {
                auto src   = ParseTermArg();
                auto index = ParseTermArg();
                return Call("Index", {src, index});
            }
            case OpCode::eDerefOf:
            {
                auto ref = ParseTermArg();
                return Call("DerefOf", {ref});
            }
            case OpCode::eLLess:
            {
                auto lhs = ParseTermArg();
                auto rhs = ParseTermArg();
                return BinOp(BinaryOpcode::eLess, lhs, rhs);
            }

            default:
            {
                if (IsNameSegment(op)) return new NameRefExpr(DecodeName());

                return new ConstantExpression(op);
            }
        }
    }

    void ParseField()
    {
        u8 next = ReadNext<u8>();
        if (!IsNameSegment(next)) return;

        s_Context.Stream.Seek(s_Context.Stream.Offset() - 1);
        auto name = DecodeName();
        LogMessage("\tNamed Field => {}\n", name);

        auto pkgLength = DecodePkgLength();
        LogMessage("\tPkgLength => {:#x}\n", pkgLength);
    }

    NameSpace* CreateNameSpace(StringView name)
    {
        NameSpace* current = nullptr;
        auto       it      = s_CurrentBlock.NameSpace->m_Children.find(name);
        if (it != s_CurrentBlock.NameSpace->m_Children.end())
            current = it->second;
        else
        {
            current = new NameSpace(name, s_CurrentBlock.NameSpace);
            s_CurrentBlock.NameSpace->m_Children[name] = current;
        }

        return current;
    }

    Object* CreateNamedObject()
    {
        u8 op;
        s_Context.Stream >> op;

        Optional<usize> initialValue = 0;
        usize           byteCount    = 0;

        Object*         object       = nullptr;
        switch (static_cast<OpCode>(op))
        {
            case OpCode::eOne: initialValue = 1; break;
            case OpCode::eZero: initialValue = 0; break;
            case OpCode::eQWordPrefix: byteCount = 4;
            case OpCode::eDWordPrefix: byteCount = 3;
            case OpCode::eWordPrefix: byteCount = 2;
            case OpCode::eBytePrefix: byteCount = 1; break;
            case OpCode::eBuffer:
            {
                auto pkgLength = DecodePkgLength();
                auto length    = ParseTermArg();

                LogMessage(
                    "Created buffer object\npkgLength => {:#x}, length => "
                    "{:#x}\n",
                    pkgLength,
                    length->Type == ExpressionType::eConstant
                        ? reinterpret_cast<ConstantExpression*>(length)->Value
                        : 0x1337);
                break;
            }
            case OpCode::ePackage:
            {
                auto pkgLength = DecodePkgLength();
                auto length = ParseTermArg();

                LogMessage(
                    "Created package object\npkgLength => {:#x}, length => "
                    "{:#x}\n",
                    pkgLength,
                    length->Type == ExpressionType::eConstant
                        ? reinterpret_cast<ConstantExpression*>(length)->Value
                        : 0x1337);

                if (IsNameSegment(PeekNextByte()))
                {
                    LogMessage("Decoded name => {}", DecodeName());
                    break;
                }
                break;
            }

            default: LogError("Failed to create object"); break;
        }

        for (usize i = 0; i < byteCount; ++i)
        {
            if (!initialValue) initialValue = 0;

            u8 next;
            s_Context.Stream >> next;

            initialValue = initialValue.Value()
                         | static_cast<u32>(next) << (4 + (i * 8));
        }

        if (initialValue) object = new IntegerObject(initialValue);

        auto str        = magic_enum::enum_name(static_cast<OpCode>(op));
        auto opCodeName = StringView(str.data(), str.size());

        LogTrace("ACPI: Created named object with an opcode => `{}`",
                 opCodeName);
        return object;
    }
    void ExecuteOpCode()
    {
        OpCode opcode = GetNextOp();
        u16   op     = ToUnderlying(opcode);
        LogMessage("{:#x}\n", op);

        switch (opcode)
        {
            case OpCode::eName:
            {
                auto name = DecodeName();
                LogMessage("NameOp => {}\n", name);

                auto nameSpace      = CreateNameSpace(name);
                nameSpace->m_Object = CreateNamedObject();
                nameSpace->m_Type   = ObjectType::eName;

                break;
            }
            case OpCode::eScope:
            {
                usize start     = s_Context.Stream.Offset();
                u32   pkgLength = DecodePkgLength();
                auto  name      = DecodeName();

                auto  current   = CreateNameSpace(name);
                current->m_Type = ObjectType::eScope;
                CodeBlock codeBlock(current, s_Context.Stream.Offset(),
                                    start + pkgLength);
                s_ScopesToParse.PushBack(codeBlock);

                s_Context.Stream.Seek(codeBlock.End);
                break;
            }
            case OpCode::eMethod:
            {
                usize start     = s_Context.Stream.Offset();
                auto  pkgLength = DecodePkgLength();
                auto  name      = DecodeName();

                auto  current   = CreateNameSpace(name);
                current->m_Type = ObjectType::eMethod;
                CodeBlock codeBlock(current, s_Context.Stream.Offset(),
                                    start + pkgLength);
                s_ScopesToParse.PushBack(codeBlock);

                s_Context.Stream.Seek(codeBlock.End);
                break;
            }
            case OpCode::eAcquire:
            {
                auto name = DecodeName();
                u16 timeout;
                s_Context.Stream >> timeout;
                break;
            }
            case OpCode::eRelease:
            {
                auto name = DecodeName();
                break;
            }
            case OpCode::eOpRegion:
            {
                auto name = DecodeName();
                break;
            }
            case OpCode::eField:
            {
                CTOS_UNUSED usize start     = s_Context.Stream.Offset();
                CTOS_UNUSED u32   pkgLength = DecodePkgLength();
                auto              name      = DecodeName();
                u8              flags;
                s_Context.Stream >> flags;

                u8                    acc          = flags & 0b1111;
                constexpr static auto s_AccStrings = ToArray<StringView>({
                    "Any",
                    "Byte",
                    "Word",
                    "DWord",
                    "QWord",
                    "Buffer",
                });

                u8                    updateRules  = flags & 0b1100000;
                ParseField();

                CTOS_UNUSED auto current = CreateNameSpace(name);
                LogMessage(
                    "Field({}, {}Acc, {}, {})\n", name, s_AccStrings[acc],
                    flags & Bit(4) ? "Lock" : "NoLock",
                    updateRules == 0
                        ? "Preserve"
                        : (updateRules == 1 ? "WriteAsOnes" : "WriteAsZeroes"));
                break;
            }
            case OpCode::eDevice:
            {
                usize     start     = s_Context.Stream.Offset();
                u32       pkgLength = DecodePkgLength();
                auto      name      = DecodeName();

                auto      current   = CreateNameSpace(name);
                CodeBlock codeBlock(current, s_Context.Stream.Offset(),
                                    start + pkgLength);
                s_ScopesToParse.PushBack(codeBlock);
                current->m_Type = ObjectType::eDevice;

                s_Context.Stream.Seek(codeBlock.End);

                break;
            }
            case OpCode::eToHexString:
            {
                auto arg0 = ParseTermArg();
                auto arg1 = ParseTermArg();
                auto statement
                    = new StoreStatement(Call("ToHexString", {arg0}), arg1);
                statement->Dump();

                break;
            }
            case OpCode::eWhile:
            {
                usize     start     = s_Context.Stream.Offset();
                auto      pkgLength = DecodePkgLength();

                auto      current   = CreateNameSpace("while");
                CodeBlock codeBlock(current, s_Context.Stream.Offset(),
                                    start + pkgLength);
                s_ScopesToParse.PushBack(codeBlock);
                current->m_Type = ObjectType::eWhile;

                s_Context.Stream.Seek(codeBlock.End);

                break;
            }
            default:
                if (IsNameSegment(op) && op)
                {
                    s_Context.Stream.Seek(s_Context.Stream.Offset() - 1);
                    LogMessage("{}()\n", DecodeName());

                    break;
                }
                LogMessage("{:#x}, {:c}\n", op, std::isalnum(op) ? op : '?');
                break;
        }
    }

    Statement* ParseStatement()
    {
        auto opcode = GetNextOp();
        u16 op     = ToUnderlying(opcode);

        switch (opcode)
        {
            case OpCode::eToHexString:
            {
                auto arg0 = ParseTermArg();
                auto arg1 = ParseTermArg();
                return new StoreStatement(Call("ToHexString", {arg0}), arg1);
            }

            case OpCode::eToBuffer:
            {
                auto arg0 = ParseTermArg();
                auto arg1 = ParseTermArg();
                return new StoreStatement(Call("ToBuffer", {arg0}), arg1);
            }

            case OpCode::eSubtract:
            {
                auto lhs = ParseTermArg();
                auto rhs = ParseTermArg();
                auto dst = ParseTermArg();
                return new StoreStatement(
                    BinOp(BinaryOpcode::eSubtract, lhs, rhs), dst);
            }
            case OpCode::eIncrement:
            {
                auto arg = ParseTermArg();
                return new StoreStatement(
                    BinOp(BinaryOpcode::eAdd, arg, new ConstantExpression(1)),
                    arg);
            }

            case OpCode::eStore:
            {
                auto src = ParseTermArg();
                auto dst = ParseSuperName();

                return new StoreStatement(src, dst);
            }

            case OpCode::eWhile:
            {
                usize start          = s_Context.Stream.Offset();
                u32   pkgLength      = DecodePkgLength();
                auto  cond           = ParseTermArg();

                auto  whileStatement = new WhileStatement(cond);
                usize bodyEnd        = start + pkgLength;

                while (s_Context.Stream.Offset() < bodyEnd)
                    whileStatement->Body.EmplaceBack(ParseStatement());
                return whileStatement;
            }

            case OpCode::eReturn:
            {
                // auto val = ParseTermArg();
                LogError("Return statement");
                // return new ReturnStatement(val);
            }

            default:
                if (IsNameSegment(op))
                {
                    auto name = DecodeName();
                    return new CallStatement(new CallExpression(name));
                }

                LogError("Unknown AML opcode: {:#x}", op);
                return nullptr;
        }
    }
    MethodObject* ParseMethod(CodeBlock codeBlock)
    {
        auto method = new MethodObject();

        s_Context.Stream.Seek(codeBlock.Start);
        u8 flags              = ReadNext<u8>();
        method->m_ArgumentCount = flags & 0b111;

        auto end                = codeBlock.End;
        while (s_Context.Stream.Offset() < end)
        {
            auto statement = ParseStatement();
            if (statement) method->m_Statements.PushBack(statement);
        }

        return method;
    }
    void ExecuteTable(ACPI::SDTHeader& table)
    {
        if (!s_RootNameSpace)
        {
            s_RootNameSpace = new NameSpace("\\");
            auto block      = CodeBlock{s_RootNameSpace, sizeof(table),
                                   table.Length - sizeof(table)};
            s_ScopesToParse.PushBack(block);
        }

        s_Context.Stream
            = ByteStream(reinterpret_cast<u8*>(&table), table.Length);

        LogInfo("ACPI: DSDT Dump =>");
        [[maybe_unused]] static usize passes = 0;

        // usize tries = 0;
        while (!s_ScopesToParse.Empty())
        {
            auto scope = s_ScopesToParse.Back();
            s_ScopesToParse.PopBack();
            // if (scope.NameSpace->m_Type != ObjectType::eMethod &&
            // !s_ScopesToParse.Empty() && tries < 3)
            // {
            //     s_ScopesToParse.PushFront(scope);
            //     ++tries;
            //     continue;
            // }
            // tries = 0;

            s_CurrentBlock = scope;
            s_Context.Stream.Seek(scope.Start);

            // LogMessage("ACPI: Parsing block =>\n");
            // LogMessage("{}({})\n{{}\n",
            // magic_enum::enum_name(scope.NameSpace->m_Type).data() + 1,
            // scope.NameSpace->m_Name);

            bool discardMethods
                = CommandLine::GetBoolean("acpi.discard-methods").ValueOr(true);
            if (scope.NameSpace->m_Type == ObjectType::eMethod
                && discardMethods)
                continue;
            auto end = scope.End;

            auto str = magic_enum::enum_name(scope.NameSpace->m_Type);
            auto objectTypeString = StringView(str.data(), str.size());
            LogTrace("ACPI::Interpreter: Parsing object `{}` of type => `{}`",
                     scope.NameSpace->m_Name, objectTypeString);
            while (s_Context.Stream.Offset() < end) ExecuteOpCode();
            LogTrace("ACPI::Interpreter: Hit end of code block");
            // LogTrace("ACPI: Parsing method...");
            // auto method = ParseMethod(scope);
            // LogTrace("ACPI: Finished parsing method.");
            // LogTrace("ACPI: Dumping method...");
            // method->Dump();
            // LogMessage("}}\n");

            // ++passes;
        }

        s_RootNameSpace->Dump();
    }
}; // namespace ACPI::Interpreter
