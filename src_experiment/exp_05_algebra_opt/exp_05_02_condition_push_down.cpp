//
// Created by Sam on 2018/2/13.
//

#include <relationalalgebra/optimizer.h>

/*
 * 等价变换：将SRA_SELECT类型的节点进行条件串接
 * 传入指针的指针作为参数便于修改多于一个值
 */
void splitAndConcatenate(SRA_s **sra_point);

/*
 * 判断表达式的操作数链表中为 TOKEN_AND
 */
bool hasLegalKeyword(Expression *pExpression);

/*
 * 拆分一个 sra 表达式，将 AND 左右分拆成子 select 语句
 */
void splitSra(SRA_s **pS);

/*
 * 当前标识符为 TOKEN_AND 时，获取当前的子表达式，也就是两个 AND
 * （或者只有一个 AND 另一个为 nullptr） 中间的子表达式，将其拆下
 * 长表达式，拆分成一个单独的 select 便于第二步中的 select 与 join
 * 的下推交换处理
 */
Expression *interceptSubexpression(Expression *pExpression);

/*
 * 用于根据传入的表达式头部与表达式尾部找到当前子表达式结束的位置
 */
Expression *goSubexpressionEnd(Expression *pFront, Expression *pBack);

/*
 * 将上一步拆分得到的 select 子语句 下推，寻找到包含 select 所需属性的 join 语句
 * 将 select 语句放至 join 之前（下方）执行，也就是先筛选需要的属性再交
 */
void conditionalExchange(SRA_t **pS, TableManager *tableManager, Transaction *transaction);

/*
 * 检查 column 名是否存在于预定的 fieldsName 中
 */
bool hasColumnNameInFieldsName(const vector<char *> &fieldsName,
                               const char *columnName);

bool hasColumnNameInSra(SRA_t *sra, Expression *pExpression, TableManager *tableManager, Transaction *transaction);

/*输入一个关系代数表达式，输出优化后的关系代数表达式
 * 要求：在查询条件符合合取范式的前提下，根据等价变换规则将查询条件移动至合适的位置。
 * */
SRA_t *dongmengdb_algebra_optimize_condition_pushdown(SRA_t *sra,
                                                      TableManager *tableManager, Transaction *transaction) {

    printf("\n=================BEGIN===============\n");
    SRA_print(sra);

    splitAndConcatenate(&sra);
    printf("\n=====================================\n");
    SRA_print(sra);

    printf("\n=====================================\n");
    conditionalExchange(&sra, tableManager, transaction);
    SRA_print(sra);
    printf("\n==================END================\n");


    return sra;
}


void splitAndConcatenate(SRA_s **sra_point) {

    // 获取传入关系代数的指针所指向的数据
    auto *sra_data = *sra_point;
    if (sra_data == nullptr) {
        return;
    }

    // 打印接收到的 SRA
//    SRA_print(sra_data);

    // 根据关系代数的类型功能分别处理
    if (sra_data->t == SRA_SELECT) {

        if (hasLegalKeyword(sra_data->select.cond)) {

            splitSra(sra_point);
        }

        splitAndConcatenate(&(sra_data->select.sra));

    } else if (sra_data->t == SRA_PROJECT) {

        splitAndConcatenate(&(sra_data->project.sra));

    } else if (sra_data->t == SRA_JOIN) {

        splitAndConcatenate(&(sra_data->join.sra1));
        splitAndConcatenate(&(sra_data->join.sra2));

    } else {
//        printf("\n\nThe above received SRA does "
//               "not specify a legal type of operation.\n\n");
    }
}

void splitSra(SRA_s **pS) {

    if (pS == nullptr) {
        return;
    }

    auto sra = *pS;
    auto express = sra->select.cond;
    auto operandListFront = express->nextexpr;
    auto operandListBack = interceptSubexpression(operandListFront);

    auto frontEnd = goSubexpressionEnd(operandListFront, operandListBack);
    frontEnd->nextexpr = nullptr;

    sra->select.sra = SRASelect(sra->select.sra, operandListFront);
    sra->select.cond = operandListBack;
}

Expression *goSubexpressionEnd(Expression *pFront, Expression *pBack) {

    auto point = pFront;

    while (point != nullptr && point->nextexpr != pBack
           && point->nextexpr != nullptr) {
        point = point->nextexpr;
    }

    return point;
}

Expression *interceptSubexpression(Expression *pExpression) {

    if (pExpression == nullptr) {
        return pExpression;
    } else if (pExpression->term == nullptr) {

        if (pExpression->opType <= TOKEN_COMMA) {

            int numOfOperators = 0;
            numOfOperators = operators[pExpression->opType].numbers;
            pExpression = pExpression->nextexpr;

            while (numOfOperators--) {
                pExpression = interceptSubexpression(pExpression);
            }

            return pExpression;
        }
    } else if (pExpression->term) {
        return pExpression->nextexpr;
    }

    return nullptr;
}

bool hasLegalKeyword(Expression *pExpression) {
    return (pExpression == nullptr) ? (false) : (pExpression->opType == TOKEN_AND);
}

bool hasColumnNameInFieldsName(const vector<char *> &fieldsName,
                               const char *columnName) {

    for (auto p : fieldsName) {
        if (strcmp(p, columnName) == 0) {
            return true;
        }
    }

    return false;
}

bool hasColumnNameInSra(SRA_t *sra, Expression *pExpression, TableManager *tableManager, Transaction *transaction) {

    auto sraType = sra->t;

    if (sraType == SRA_SELECT) {

        return hasColumnNameInSra(sra->select.sra, pExpression, tableManager, transaction);

    } else if (sraType == SRA_JOIN) {

//        return (hasColumnNameInSra(sra->join.sra1, pExpression, tableManager, transaction)
//                | hasColumnNameInSra(sra->join.sra2, pExpression, tableManager, transaction));
        auto status = hasColumnNameInSra(sra->join.sra1, pExpression, tableManager, transaction);

        return status ? (status) : (hasColumnNameInSra(sra->join.sra2, pExpression, tableManager, transaction));

    } else if (sraType == SRA_TABLE) {

        for (auto point = pExpression; point != nullptr; point = point->nextexpr) {

            if (point->term != nullptr && point->term->t == TERM_COLREF) {

                if (point->term->ref->tableName != nullptr) {

                    auto debug_1 = point->term->ref->tableName;
                    auto debug_2 = point->term->ref->columnName;
                    auto debug_3 = sra->table.ref->table_name;

                    if (strcmp(point->term->ref->tableName,
                               sra->table.ref->table_name) == 0) {

                        return true;
                    }
                } else {

                    auto fields = tableManager->table_manager_get_tableinfo(
                            sra->table.ref->table_name, transaction);

                    if (hasColumnNameInFieldsName(fields->fieldsName,
                                                   point->term->ref->columnName)) {
                        return true;
                    }
                }
            }
        }

        return false;

    }

    return false;
}

void conditionalExchange(SRA_t **pS, TableManager *tableManager, Transaction *transaction) {

    auto sra = *pS;

    if (sra == nullptr) {
        return;
    } else if (sra->t == SRA_SELECT) {

        conditionalExchange(&(sra->select.sra), tableManager, transaction);

        auto selectSra = sra->select.sra;

        // 把两个 select 倒过来了
        if (selectSra->t == SRA_SELECT) {

            sra->select.sra = selectSra->select.sra;
            selectSra->select.sra = sra;
            *pS = selectSra;

            conditionalExchange(&(selectSra->select.sra), tableManager, transaction);
//            conditionalExchange(&(sra->select.sra), tableManager, transaction);

        } else if (selectSra->t == SRA_JOIN) {

            auto leftBranch = hasColumnNameInSra(selectSra->join.sra1,
                                                 sra->select.cond, tableManager, transaction),
                    rightBranch = hasColumnNameInSra(selectSra->join.sra2,
                                                     sra->select.cond, tableManager, transaction);

            if (leftBranch && !rightBranch) {

                sra->select.sra = selectSra->join.sra1;
                selectSra->join.sra1 = sra;
                *pS = selectSra;

                conditionalExchange(&(selectSra->join.sra1), tableManager, transaction);
//                conditionalExchange(&(sra.))

            } else if (!leftBranch && rightBranch) {

                sra->select.sra = selectSra->join.sra2;
                selectSra->join.sra2 = sra;
                *pS = selectSra;

                conditionalExchange(&(selectSra->join.sra2), tableManager, transaction);

            }
        }

    } else if (sra->t == SRA_PROJECT) {

        conditionalExchange(&(sra->project.sra), tableManager, transaction);

    } else if (sra->t == SRA_JOIN) {

        conditionalExchange(&(sra->join.sra1), tableManager, transaction);
        conditionalExchange(&(sra->join.sra2), tableManager, transaction);

    }

}