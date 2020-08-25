#include "test.h"

MJB_EXPORT void mjb_ready_test() {
    mjb_assert("Not ready", !mjb_ready());

    bool result = mjb_initialize("null.db");
    mjb_assert("Not valid DB call", !result);
    mjb_assert("Not valid DB", !mjb_ready());

    result = mjb_close();
    mjb_assert("Not valid DB close call", !result);
    mjb_assert("DB closed", !mjb_ready());

    result = mjb_initialize(MJB_DB_PATH);
    mjb_assert("Valid DB call", result);
    mjb_assert("Valid DB", mjb_ready());

    result = mjb_close();
    mjb_assert("Valid DB close call", result);
    mjb_assert("DB closed", !mjb_ready());
}
