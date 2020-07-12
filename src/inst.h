#ifndef INST_H
#define INST_H

#include "p_conversions.h"
#include "p_interaction.h"
#include "p_logic.h"
#include "p_operands.h"
#include "p_create.h"
#include "p_objects.h"
#include "p_property.h"
#include "p_stack.h"
#include "p_strings.h"
#include "p_tests.h"
#include "p_time.h"
#include "p_descriptor.h"
#include "p_for.h"

#define TOTALLEN (PRIMS_CONVERSIONS_LEN + PRIMS_INTERACTION_LEN + \
        PRIMS_LOGIC_LEN + PRIMS_OPERANDS_LEN + PRIMS_OBJECTS_LEN + \
        PRIMS_CREATE_LEN + PRIMS_PROPERTY_LEN + PRIMS_STACK_LEN + \
        PRIMS_STRINGS_LEN + PRIMS_TESTS_LEN + PRIMS_TIME_LEN + \
        PRIMS_DESCRIPTOR_LEN + PRIMS_FOR_LEN)

#define IN_FOR_ADD (TOTALLEN - 2)  /*kludge! prims_for MUST be last!*/
#define IN_FOR_CHECK (TOTALLEN - 1)
#define IN_FOR_POP (TOTALLEN)
#define IN_IF (TOTALLEN + 1)
#define IN_CALL (TOTALLEN + 2)
#define IN_READ (TOTALLEN + 3)
#define IN_RET (TOTALLEN + 4)
#define IN_JMP (TOTALLEN + 5)
#define IN_PROGRAM (TOTALLEN + 6)
#define IN_EXECUTE (TOTALLEN + 7)
#define IN_SLEEP (TOTALLEN + 8)
#define IN_VAR (TOTALLEN + 9)
#define IN_LOOP (TOTALLEN + 10)
#define IN_NOP (TOTALLEN + 11)

#define BASE_MIN 1
#define BASE_MAX (TOTALLEN + 11)

/* now refer to tables to map instruction number to name */
extern char *base_inst[];

extern char *insttotext(inst *theinst);
extern char *insttoerr(inst *theinst);
/* and declare debug instruction diagnostic routine */
extern char *debug_inst(inst *pc, inst *stack1, int sp);

#endif /* INST_H */
