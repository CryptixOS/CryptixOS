/*
 * Created by v1tr10l7 on 25.02.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Prism/Debug/Log.hpp>
#include <Prism/Types.hpp>

namespace
{
    struct SourceLocation
    {
        const char* File;
        u32         Line;
        u32         Column;
    };

    struct TypeDescriptor
    {
        u16  Kind;
        u16  Info;
        char Name[];
    };

    struct OverflowData
    {
        SourceLocation  Location;
        TypeDescriptor* Type;
    };

    struct ShiftOutOfBoundsData
    {
        SourceLocation  Location;
        TypeDescriptor* LeftType;
        TypeDescriptor* RightType;
    };

    struct InvalidValueData
    {
        SourceLocation  Location;
        TypeDescriptor* Type;
    };

    struct ArrayOutOfBoundsData
    {
        SourceLocation  Location;
        TypeDescriptor* ArrayType;
        TypeDescriptor* IndexType;
    };

    struct TypeMismatchV1Data
    {
        SourceLocation  Location;
        TypeDescriptor* Type;
        u8              LogAlignment;
        u8              TypeCheckKind;
    };

    struct FunctionTypeMismatchV1Data
    {
        SourceLocation  Location;
        TypeDescriptor* Type;
        u8              LogAlignment;
        u8              TypeCheckKind;
    };

    struct NegativeVlaData
    {
        SourceLocation  Location;
        TypeDescriptor* Type;
    };

    struct NonNullReturnData
    {
        SourceLocation Location;
    };

    struct NonNullArgdata
    {
        SourceLocation Location;
    };

    struct UnreachableData
    {
        SourceLocation Location;
    };

    struct InvalidBuiltinData
    {
        SourceLocation Location;
        u8             Kind;
    };

    struct FloatCastOverflowData
    {
        SourceLocation Location;
    };

    struct MissingReturnData
    {
        SourceLocation Location;
    };

    struct AlignmentAssumptionData
    {
        SourceLocation  Location;
        SourceLocation  AssumptionLocation;
        TypeDescriptor* Type;
    };
} // namespace

static void Warn(const char* message, SourceLocation source)
{
    Prism::Log::Warn("Ubsan: {} ->\n{}[{}:{}]", message, source.File,
                     source.Line, source.Column);
    // Stacktrace::Print(4);
}

extern "C"
{
    void __ubsan_handle_add_overflow(OverflowData* data)
    {
        Warn("addition overflow", data->Location);
    }

    void __ubsan_handle_sub_overflow(OverflowData* data)
    {
        Warn("subtraction overflow", data->Location);
    }

    void __ubsan_handle_mul_overflow(OverflowData* data)
    {
        Warn("multiplication overflow", data->Location);
    }

    void __ubsan_handle_divrem_overflow(OverflowData* data)
    {
        Warn("division overflow", data->Location);
    }

    void __ubsan_handle_negate_overflow(OverflowData* data)
    {
        Warn("negation overflow", data->Location);
    }

    void __ubsan_handle_pointer_overflow(OverflowData* data)
    {
        Warn("pointer overflow", data->Location);
    }

    void __ubsan_handle_shift_out_of_bounds(ShiftOutOfBoundsData* data)
    {
        Warn("shift out of bounds", data->Location);
    }

    void __ubsan_handle_load_invalid_value(InvalidValueData* data)
    {
        Warn("invalid load value", data->Location);
    }

    void __ubsan_handle_out_of_bounds(ArrayOutOfBoundsData* data)
    {
        Warn("array out of bounds", data->Location);
    }

    void __ubsan_handle_type_mismatch_v1(TypeMismatchV1Data* data,
                                         uintptr_t           ptr)
    {
        if (ptr == 0) Warn("use of nullptr", data->Location);
        else if (ptr & ((1 << data->LogAlignment) - 1))
            Warn("unaligned access", data->Location);
        else Warn("no space for object", data->Location);
    }

    void __ubsan_handle_function_type_mismatch(FunctionTypeMismatchV1Data* data,
                                               uintptr_t                   ptr)
    {
        Warn("call to a function through pointer to incorrect function",
             data->Location);
    }

    void __ubsan_handle_function_type_mismatch_v1(
        FunctionTypeMismatchV1Data* data, uintptr_t ptr, uintptr_t calleeRTTI,
        uintptr_t fnRTTI)
    {
        Warn("call to a function through pointer to incorrect function",
             data->Location);
    }

    void __ubsan_handle_vla_bound_not_positive(NegativeVlaData* data)
    {
        Warn("variable-length argument is negative", data->Location);
    }

    void __ubsan_handle_nonnull_return(NonNullReturnData* data)
    {
        Warn("non-null return is null", data->Location);
    }

    void __ubsan_handle_nonnull_return_v1(NonNullReturnData* data)
    {
        Warn("non-null return is null", data->Location);
    }

    void __ubsan_handle_nonnull_arg(NonNullArgdata* data)
    {
        Warn("non-null argument is null", data->Location);
    }

    void __ubsan_handle_builtin_unreachable(UnreachableData* data)
    {
        Warn("unreachable code reached", data->Location);
    }

    void __ubsan_handle_invalid_builtin(InvalidBuiltinData* data)
    {
        Warn("invalid builtin", data->Location);
    }

    void __ubsan_handle_float_cast_overflow(FloatCastOverflowData* data)
    {
        Warn("float cast overflow", data->Location);
    }

    void __ubsan_handle_missing_return(MissingReturnData* data)
    {
        Warn("missing return", data->Location);
    }

    void __ubsan_handle_alignment_assumption(AlignmentAssumptionData* data)
    {
        Warn("alignment assumption", data->Location);
    }
};
