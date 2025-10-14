/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

/* Windows fixes */
#if defined(_MSC_VER)
/*
** Remove all nodes that are part of expression pExpr from the rename list.
*/
SQLITE_PRIVATE void sqlite3RenameExprUnmap(Parse *pParse, Expr *pExpr) {
    // Fake function for MSVC compatibility

    return;
}

SQLITE_PRIVATE void sqlite3RenameExprlistUnmap(Parse *pParse, ExprList *pEList) {
    // Fake function for MSVC compatibility

    return;
}

SQLITE_PRIVATE void sqlite3RenameTokenRemap(Parse *pParse, const void *pTo, const void *pFrom) {
    // Fake function for MSVC compatibility
}

#endif
