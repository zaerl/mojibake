/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

/* Windows fixes */
#if defined(_MSC_VER)

SQLITE_PRIVATE const void *sqlite3RenameTokenMap( Parse *pParse, const void *pPtr,
    const Token *pToken) {
    return NULL;
}

/*
** Remove all nodes that are part of expression pExpr from the rename list.
*/
SQLITE_PRIVATE void sqlite3RenameExprUnmap(Parse *pParse, Expr *pExpr) {
    return;
}

SQLITE_PRIVATE void sqlite3RenameExprlistUnmap(Parse *pParse, ExprList *pEList) {
    return;
}

SQLITE_PRIVATE void sqlite3RenameTokenRemap(Parse *pParse, const void *pTo, const void *pFrom) {
    return;
}

SQLITE_PRIVATE void sqlite3AlterRenameColumn(Parse *pParse, SrcList *pSrc, Token *pOld,
    Token *pNew) {
    return;
}

SQLITE_PRIVATE void sqlite3AlterBeginAddColumn(Parse *pParse, SrcList *pSrc) {
    return;
}

#endif
