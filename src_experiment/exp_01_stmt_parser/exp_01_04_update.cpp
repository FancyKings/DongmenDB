//
// Created by Sam on 2018/2/13.
//

#include <dongmensql/sqlstatement.h>
#include <parser/StatementParser.h>

/**
 * 在现有实现基础上，实现update from子句
 *
 * 支持的update语法：
 *
 * UPDATE <table_name> SET <field1> = <expr1>[, <field2 = <expr2>, ..]
 * WHERE <logical_expr>
 *
 * 解析获得 sql_stmt_update 结构
 */

/* TODO: parse_sql_stmt_update， update语句解析 */
// update student set sage=20[,fieldname=expr] where sage=22
sql_stmt_update *UpdateParser::parse_sql_stmt_update() {
//    fprintf(stderr, \
//    "TODO: update is in debug yet. in parse_sql_stmt_update \n");

    // 存储 数据表 名称
    char *tableName = nullptr;

    // 存储 键 集
    auto fields = new vector<char *>;

    // 存储 值 集
    auto fieldsExpr = new vector<Expression *>;

    // 匹配 where 条件，限定作用域
    SRA_t *where = nullptr;

    //  匹配 update，若匹配到则迭代到下一单词，否则返回 0
    Token *token = parseNextToken();
    if (matchToken(TOKEN_RESERVED_WORD, "update") == 0) {
        strcpy(this->parserMessage, \
        "ERROR UPDATE SQL PARSE IN parse_sql_update \n");
        return nullptr;
    }

    // 获取表名
    token = parseNextToken();
    if (token->type == TOKEN_WORD) {
        tableName = new_id_name();
        strcpy(tableName, token->text);
    } else {
        strcpy(this->parserMessage, "ERROR SQL: MISSING TABLE NAME \n");
        return nullptr;
    }

    // 匹配 set
    // 此时 currToken 肯定不为 NULL 且并非我们所需要的数据，所以吃掉读取下一个
    token = parseEatAndNextToken();
    if (matchToken(TOKEN_RESERVED_WORD, "set") == 0) {
        strcpy(this->parserMessage, "ERROR SQL: MISSING SET");
        return nullptr;
    }

    // 欲设置的键值对匹配
    token = parseNextToken();
    while (token != nullptr && (token->type == TOKEN_WORD ||
        token->type == TOKEN_COMMA)) {
        if(token->type == TOKEN_COMMA) {
            token = parseEatAndNextToken();
        }

        // 匹配字段名
        if (token->type == TOKEN_WORD) {
            char *fieldName = new_id_name();
            strcpy(fieldName, token->text);
            fields->push_back(fieldName);
        } else {
            strcpy(this->parserMessage, "ERROR SQL: INVALID FIELD NAME \n");
            return nullptr;
        }

        // 同理，跳过已经检查完毕的非NULL单词
        parseEatAndNextToken();

        // 匹配 等号，标志下一步存入的是字段的值
        if (matchToken(TOKEN_EQ, "=") == 0) {
            strcpy(this->parserMessage, "ERROR SQL: NO EQUAL \n");
            return nullptr;
        }

        // 匹配 值
        Expression *exp = parseExpressionRD();
        fieldsExpr->push_back(exp);
        token = parseNextToken();
    }

    // 进行 where 限定域的查找
    where = SRATable(TableReference_make(tableName, nullptr));
    if (token != nullptr) {
        if (matchToken(TOKEN_RESERVED_WORD, "where") == 0) {
            strcpy(this->parserMessage, "ERROR SQL: NO WHERE ELEMENT \n");
            return nullptr;
        }
        // printf("Update Paser OK \n");
        Expression *cond = parseExpressionRD();
        where = SRASelect(where, cond);
    }

    // 构造返回值
    sql_stmt_update * sqlStmtUpdate = (sql_stmt_update *)
            calloc(1, sizeof(sql_stmt_update));
    sqlStmtUpdate->tableName = tableName;
    sqlStmtUpdate->fields.assign(fields->begin(), fields->end());
    sqlStmtUpdate->fieldsExpr.assign(fieldsExpr->begin(), fieldsExpr->end());
    sqlStmtUpdate->where = where;

    return sqlStmtUpdate;
};