//
// Created by Sam on 2018/2/13.
//

#include <dongmensql/sqlstatement.h>
#include <parser/StatementParser.h>

/**
 * 在现有实现基础上，实现delete from子句
 *
 *  支持的delete语法：
 *
 *  DELETE FROM <table_nbame>
 *  WHERE <logical_expr>
 *
 * 解析获得 sql_stmt_delete 结构
 */

sql_stmt_delete *DeleteParser::parse_sql_stmt_delete(){

    char *tableName = nullptr;

    Token *token = parseNextToken();

    // 匹配关键字 delete
    if (matchToken(TOKEN_RESERVED_WORD, "delete") == 0) {
        strcpy(this->parserMessage, \
        "DELETE statement parsing error: did not match the keyword DELETE \n");
        return nullptr;
    }

    // 匹配表名
    token = parseNextToken();
    if (token->type == TOKEN_WORD) {
        tableName = new_id_name();
        strcpy(tableName, token->text);
    } else {
        strcpy(this->parserMessage, \
        "Delete statement interpretation error: did not match the TABLE NAME \n");
        return nullptr;
    }

    parseEatAndNextToken();

    if (matchToken(TOKEN_RESERVED_WORD, "where") == 0) {
        strcpy(this->parserMessage, \
        "Delete statement parsing failed: did not match the keyword WHERE \n");
        return nullptr;
    }

    // 解析 where 表达式
    SRA_t *where = SRATable(TableReference_make(tableName, nullptr));
    Expression *cond = parseExpressionRD();
    where = SRASelect(where, cond);

    // 构造返回值
    sql_stmt_delete *sqlStmtDelete = nullptr;
    sqlStmtDelete = (sql_stmt_delete *) calloc(sizeof(sql_stmt_delete), 1);
    sqlStmtDelete->tableName = tableName;
    sqlStmtDelete->where = where;

    return sqlStmtDelete;
};
