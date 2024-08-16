#include <cstdint>
#include <util/logger.hpp>
#include <utility>

struct type_descriptor {
    uint16_t type_kind;
    uint16_t type_info;
    char *type_name;
};

struct source_location {
    char *filename;
    uint32_t line;
    uint32_t col;
};

struct base {
    source_location loc;
};

struct type_mismatch_v1 : base {
    type_descriptor *type;
    uint8_t log_align;
    uint8_t type_check_kind;
};

struct overflow : base {
    type_descriptor *type;
};

struct out_of_bounds : base {
    type_descriptor *array_type;
    type_descriptor *index_type;
};

struct invalid_value : base {
    type_descriptor *type;
};

struct shift_out_of_bounds : base {
    type_descriptor *lhs_type;
    type_descriptor *rhs_type;
};

struct invalid_builtin : base {
    uint8_t kind;
};

typedef base unreachable;
typedef base nonnull_arg;

void log_location(source_location& loc) {
    logger::error("UBSAN", "Failure at %s:%u", loc.filename, loc.line);
}

extern "C" {
    void __ubsan_handle_type_mismatch_v1(type_mismatch_v1 *data, uintptr_t ptr) {
        log_location(data->loc);
        if (!ptr)
            logger::panic("UBSAN[TYPE_MM]", "Null pointer access");
        else if (ptr & ((1 << data->log_align) - 1))
            logger::panic("UBSAN[TYPE_MM]", "Use of misaligned pointer");
        else
            logger::panic("UBSAN[TYPE_MM]", "Insufficient space for object");
    }

    void __ubsan_handle_pointer_overflow(overflow *data, uintptr_t base, uintptr_t result) {
        log_location(data->loc);
        logger::panic("UBSAN[PTR_OVF]", "Pointer overflow from %p to %p", base, result);
    }

    void __ubsan_handle_out_of_bounds(out_of_bounds *data, uintptr_t) {
        log_location(data->loc);
        logger::panic("UBSAN[IDX_OOB]", "Array index out of bounds");
    }

    void __ubsan_handle_load_invalid_value(invalid_value *data, uintptr_t) {
        log_location(data->loc);
        logger::panic("UBSAN[INVL_LOAD]", "Load of invalid value");
    }

    void __ubsan_handle_mul_overflow(overflow *data, uintptr_t, uintptr_t) {
        log_location(data->loc);
        logger::panic("UBSAN[MUL]", "Multiplication overflow");
    }

    void __ubsan_handle_add_overflow(overflow *data, uintptr_t, uintptr_t) {
        log_location(data->loc);
        logger::panic("UBSAN[ADD]", "Addition overflow");
    }

    void __ubsan_handle_sub_overflow(overflow *data, uintptr_t, uintptr_t) {
        log_location(data->loc);
        logger::panic("UBSAN[SUB]", "Subtraction overflow");
    }

    void __ubsan_handle_divrem_overflow(overflow *data, uintptr_t, uintptr_t) {
        log_location(data->loc);
        logger::panic("UBSAN[DIV]", "Division overflow");
    }

    void __ubsan_handle_shift_out_of_bounds(shift_out_of_bounds *data, uintptr_t, uintptr_t) {
        log_location(data->loc);
        logger::panic("UBSAN[SHIFT_OOB]", "Shift out of bounds");
    }

    void __ubsan_handle_negate_overflow(overflow *data, uintptr_t) {
        log_location(data->loc);
        logger::panic("UBSAN[NEG_OVF]", "Negation overflow");
    }

    void __ubsan_handle_nonnull_arg(nonnull_arg *data) {
        log_location(data->loc);
        logger::panic("UBSAN[NN_ARG]", "Non null argument is null");
    }

    void __ubsan_handle_invalid_builtin(invalid_builtin *data) {
        log_location(data->loc);
        logger::panic("UBSAN[BLTIN]", "Invalid invocation of builtin");
    }

    void __ubsan_handle_builtin_unreachable(unreachable *data) {
        log_location(data->loc);
        logger::panic("UBSAN[BLTIN]", "Builtin unreachable was reached");
    }

    void __ubsan_handle_missing_return(unreachable *data) {
        log_location(data->loc);
        logger::panic("UBSAN[MISS_RET]", "Missing return");
    }
}