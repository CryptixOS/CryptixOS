/*
 * Created by v1tr10l7 on 21.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "Common.hpp"

namespace
{
    struct source_location
    {
        const char* file;
        u32         line;
        u32         column;
    };

    struct type_descriptor
    {
        u16  kind;
        u16  info;
        char name[];
    };

    struct overflow_data
    {
        source_location  location;
        type_descriptor* type;
    };

    struct shift_out_of_bounds_data
    {
        source_location  location;
        type_descriptor* left_type;
        type_descriptor* right_type;
    };

    struct invalid_value_data
    {
        source_location  location;
        type_descriptor* type;
    };

    struct array_out_of_bounds_data
    {
        source_location  location;
        type_descriptor* array_type;
        type_descriptor* index_type;
    };

    struct type_mismatch_v1_data
    {
        source_location  location;
        type_descriptor* type;
        u8               log_alignment;
        u8               type_check_kind;
    };

    struct function_type_mismatch_v1_data
    {
        source_location  location;
        type_descriptor* type;
        u8               log_alignment;
        u8               type_check_kind;
    };

    struct negative_vla_data
    {
        source_location  location;
        type_descriptor* type;
    };

    struct nonnull_return_data
    {
        source_location location;
    };

    struct nonnull_arg_data
    {
        source_location location;
    };

    struct unreachable_data
    {
        source_location location;
    };

    struct invalid_builtin_data
    {
        source_location location;
        u8              kind;
    };

    struct float_cast_overflow_data
    {
        source_location location;
    };

    struct missing_return_data
    {
        source_location location;
    };

    struct alignment_assumption_data
    {
        source_location  location;
        source_location  assumption_location;
        type_descriptor* type;
    };

    [[maybe_unused]]
    void Log(const char* message, source_location source)
    {
        LogWarn("Ubsan: {} ->\n{}[{}:{}]", message, source.file, source.line,
                source.column);
        Stacktrace::Print(4);
    }
} // namespace

extern "C"
{
    void __ubsan_handle_add_overflow(overflow_data* data)
    {
        Log("addition overflow", data->location);
    }

    void __ubsan_handle_sub_overflow(overflow_data* data)
    {
        Log("subtraction overflow", data->location);
    }

    void __ubsan_handle_mul_overflow(overflow_data* data)
    {
        Log("multiplication overflow", data->location);
    }

    void __ubsan_handle_divrem_overflow(overflow_data* data)
    {
        Log("division overflow", data->location);
    }

    void __ubsan_handle_negate_overflow(overflow_data* data)
    {
        Log("negation overflow", data->location);
    }

    void __ubsan_handle_pointer_overflow(overflow_data* data)
    {
        Log("pointer overflow", data->location);
    }

    void __ubsan_handle_shift_out_of_bounds(shift_out_of_bounds_data* data)
    {
        Log("shift out of bounds", data->location);
    }

    void __ubsan_handle_load_invalid_value(invalid_value_data* data)
    {
        Log("invalid load value", data->location);
    }

    void __ubsan_handle_out_of_bounds(array_out_of_bounds_data* data)
    {
        Log("array out of bounds", data->location);
    }

    void __ubsan_handle_type_mismatch_v1(type_mismatch_v1_data* data,
                                         uintptr_t              ptr)
    {
        if (ptr == 0) Log("use of nullptr", data->location);
        else if (ptr & ((1 << data->log_alignment) - 1))
            Log("unaligned access", data->location);
        else Log("no space for object", data->location);
    }

    void
    __ubsan_handle_function_type_mismatch(function_type_mismatch_v1_data* data,
                                          uintptr_t                       ptr)
    {
        Log("call to a function through pointer to incorrect function",
            data->location);
    }

    void __ubsan_handle_function_type_mismatch_v1(
        function_type_mismatch_v1_data* data, uintptr_t ptr,
        uintptr_t calleeRTTI, uintptr_t fnRTTI)
    {
        Log("call to a function through pointer to incorrect function",
            data->location);
    }

    void __ubsan_handle_vla_bound_not_positive(negative_vla_data* data)
    {
        Log("variable-length argument is negative", data->location);
    }

    void __ubsan_handle_nonnull_return(nonnull_return_data* data)
    {
        Log("non-null return is null", data->location);
    }

    void __ubsan_handle_nonnull_return_v1(nonnull_return_data* data)
    {
        Log("non-null return is null", data->location);
    }

    void __ubsan_handle_nonnull_arg(nonnull_arg_data* data)
    {
        Log("non-null argument is null", data->location);
    }

    void __ubsan_handle_builtin_unreachable(unreachable_data* data)
    {
        Log("unreachable code reached", data->location);
    }

    void __ubsan_handle_invalid_builtin(invalid_builtin_data* data)
    {
        Log("invalid builtin", data->location);
    }

    void __ubsan_handle_float_cast_overflow(float_cast_overflow_data* data)
    {
        Log("float cast overflow", data->location);
    }

    void __ubsan_handle_missing_return(missing_return_data* data)
    {
        Log("missing return", data->location);
    }

    void __ubsan_handle_alignment_assumption(alignment_assumption_data* data)
    {
        Log("alignment assumption", data->location);
    }
};
