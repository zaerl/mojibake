#include "test.h"

MJB_EXPORT void mjb_ready_test() {
    mojibake *mjb;

    mjb_assert("Not ready", !mjb_ready(NULL));

    bool result = mjb_initialize("null.db", &mjb);
    mjb_assert("Not valid DB call", !result && mjb != NULL);
    mjb_assert("Not valid DB", !mjb_ready(mjb));

    result = mjb_close(mjb);
    mjb_assert("Not valid DB close call", !result);
    mjb_assert("DB closed", !mjb_ready(mjb));

    result = mjb_initialize(MJB_DB_PATH, &mjb);
    mjb_assert("Valid DB call", result);
    mjb_assert("Valid DB", mjb_ready(mjb));

    result = mjb_close(mjb);
    mjb_assert("Valid DB close call", result);
    mjb_assert("DB closed", !mjb_ready(mjb));
}
