//
// Created by flassabe on 19/11/2021.
//

#include "check.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/limits.h>

#include "table.h"

/*!
 * @brief function check_query is the high level check function, which will call specialized check functions and
 * return their result.
 * @param query a pointer to a query result structure returned after parsing the SQL query (@see sql.h)
 * @return true if the query is valid, false else
 */
bool check_query(query_result_t *query) {
    switch (query->query_type) {
        case QUERY_SELECT:
            return check_query_select(&query->query_content.select_query);
            break;
        case QUERY_INSERT:
            return check_query_insert(&query->query_content.insert_query);
            break;
        case QUERY_CREATE_TABLE:
            return check_query_create(&query->query_content.create_query);
            break;
        case QUERY_UPDATE:
            return check_query_update(&query->query_content.update_query);
            break;
        case QUERY_DELETE:
            return check_query_delete(&query->query_content.delete_query);
            break;
        case QUERY_DROP_TABLE:
            return check_query_drop_table(query->query_content.table_name);
            break;
        case QUERY_DROP_DB:
            return check_query_drop_db(query->query_content.database_name);
            break;

        default:
            printf("Unsupported query code\n");
    }
    return false;
}

/*!
 * @brief check_query_select checks a select query
 * It relies on check_fields_list (list of fields to display and WHERE clause) and check_value_types (WHERE clause)
 * Also uses get_table_definition from table.h
 * @see check_fields_list
 * @see check_value_types
 * @see get_table_definition
 * @param query a pointer to the select query
 * @return true if valid, false if invalid
 */
bool check_query_select(update_or_select_query_t *query) {
    table_definition_t table_definition;
    table_definition_t *retour_definition = get_table_definition(query->table_name, &table_definition);

    if (table_exists(query->table_name)) { //Si la table existe
        if (((query->set_clause.fields_count == 1) && (strcmp(query->set_clause.fields[0].column_name, "*") == 0)) || 
            check_fields_list(&query->set_clause, &table_definition)) { //Si les champs de la liste de champs existent tous
            if (query->where_clause.values.fields_count == 0) {
                return true; //S'il n'y a pas de clause where
            } else {
                if (query->where_clause.logic_operator != OP_ERROR) {
                    if (check_fields_list(&query->where_clause.values, &table_definition)) { //Si les champs de la liste de champs dans le where existent tous
                        if (check_value_types(&query->where_clause.values, &table_definition)) { //Si les types dans le where sont correctes
                            return true;
                        }
                    }
                } else {
                    printf("L'opération effectuée dans la clause WHERE n'est pas conforme\n");
                }
            }
        }
    } else {
        printf("La table %s n'existe pas\n", query->table_name);
    }
    return false;
}

/*!
 * @brief check_query_update checks an update query
 * It relies on check_value_types and get_table_definition from table.h.
 * @see check_value_types
 * @see get_table_definition
 * @param query a pointer to the update query
 * @return true if valid, false if invalid
 */
bool check_query_update(update_or_select_query_t *query) {
    table_definition_t table_definition;
    table_definition_t *retour_definition = get_table_definition(query->table_name, &table_definition);
    
    if (table_exists(query->table_name)) { //Si la table existe
        if (check_fields_list(&query->set_clause, &table_definition)) { //Si les champs du set existent tous
            if (check_value_types(&query->set_clause, &table_definition)) { //Si les types des champs du set sont bons
                if (query->where_clause.values.fields_count == 0) {
                    return true; //S'il n'y a pas de clause where
                } else {
                    if (query->where_clause.logic_operator != OP_ERROR) {
                        if (check_fields_list(&query->where_clause.values, &table_definition)) { //Si les champs de la liste de champs dans le where existent tous
                            if (check_value_types(&query->where_clause.values, &table_definition)) { //Si les types dans le where sont correctes
                                return true;
                            }
                        }
                    } else {
                        printf("L'opération effectuée dans la clause WHERE n'est pas conforme\n");
                    }
                }
            }
        }
    } else {
        printf("La table %s n'existe pas\n", query->table_name);
    }

    return false;
}

/*!
 * @brief check_query_create checks a create query. It checks if the table doesn't already exist
 * @param query a pointer to the create query
 * @return true if valid, false if invalid
 */
bool check_query_create(create_query_t *query) {
    if (!table_exists(query->table_name)) { //Si la table n'existe pas
        return true;
    }
    printf("La table %s existe déjà\n", query->table_name);
    return false;
}

/*!
 * @brief check_query_insert checks an insert query
 * It relies on check_value_types and get_table_definition from table.h.
 * Prior to checking the field/value pairs, you must join fields and values (extracted to different table_record_t
 * structs during the SQL parsing) by copying the values strings into the text_value member of the fields names
 * table_record_t structure.
 * @see check_value_types
 * @see get_table_definition
 * @param query a pointer to the insert query
 * @return true if valid, false if invalid
 */
bool check_query_insert(insert_query_t *query) {
    table_definition_t table_definition;
    table_definition_t *retour_definition = get_table_definition(query->table_name, &table_definition);
    
    if (table_exists(query->table_name)) { //Si la table existe
        if (check_fields_list(&query->fields_names, &table_definition)) { //Si les champs avant VALUES existes tous
            if (query->fields_names.fields_count == query->fields_values.fields_count) { //Si le nombre de champs est égal au nombre de valeurs
                for (int i=0; i<query->fields_names.fields_count; i++){ //On copie les valeurs dans la meme structure que la liste des noms de champs
                    strcpy(query->fields_names.fields[i].field_value.text_value, query->fields_values.fields[i].field_value.text_value);
                }
                if (check_value_types(&query->fields_names, &table_definition)) { //Si les types des valeurs après VALUES sont bons
                    return true;
                }
            }
        }
    } else {
        printf("La table %s n'existe pas\n", query->table_name);
    }
    
    return false;
}

/*!
 * @brief check_query_delete checks a delete query
 * It relies on check_value_types (WHERE clause) and get_table_definition from table.h.
 * @see check_value_types
 * @see get_table_definition
 * @param query a pointer to the delete query
 * @return true if valid, false if invalid
 */
bool check_query_delete(delete_query_t *query) {
    table_definition_t table_definition;
    table_definition_t *retour_definition = get_table_definition(query->table_name, &table_definition);
    
    if (table_exists(query->table_name)) { //Si la table existe
        if (query->where_clause.values.fields_count == 0) {
            return true; //S'il n'y a pas de clause where
        } else {
            if (query->where_clause.logic_operator != OP_ERROR) {
                if (check_fields_list(&query->where_clause.values, &table_definition)) { //Si les champs de la liste de champs dans le where existent tous
                    if (check_value_types(&query->where_clause.values, &table_definition)) { //Si les types dans le where sont correctes
                        return true;
                    }
                }
            } else {
                printf("L'opération effectuée dans la clause WHERE n'est pas conforme\n");
            }
        }
    } else {
        printf("La table %s n'existe pas\n", query->table_name);
    }

    return false;
}

/*!
 * @brief check_query_drop_table checks a drop table query: checks that the table exists
 * @param table_name the table name
 * @return true if valid, false if invalid
 */
bool check_query_drop_table(char *table_name) {
    if (table_exists(table_name)) {
        return true;
    }
    return false;
}

/*!
 * @brief check_query_drop_db checks a drop database query: checks that the database exists
 * @param db_name the database name
 * @return true if valid, false if invalid
 */
bool check_query_drop_db(char *db_name) {
    char buffer[300];
    char *chemin;
    char *parent;
    chemin = getcwd(buffer, sizeof(buffer)); //On récupère le répertoire courant
    parent = strstr(chemin+strlen(chemin)-4, "/db"); //On test si le dernier répertoire est "db"
    if ((parent == NULL) || (strcmp(parent, "/db") != 0)) { //Si on est pas dans "db"
        if (directory_exists(db_name)) {
            return true;
        }
    } else {
        return true;
    }
    
    return false;
}

/*!
 * @brief function check_fields_list checks if all fields from a fields list exist in a table definition
 * Uses find_field_definition
 * @see find_field_definition
 * @param fields_list list of the fields to check
 * @param table_definition table definition within which to check
 * @return true if all fields belong to table, false else
 */
bool check_fields_list(table_record_t *fields_list, table_definition_t *table_definition) {
    bool response = false;
    for (int i=0; i<fields_list->fields_count; i++) {
        if (find_field_definition(&fields_list->fields[i].column_name, table_definition) != NULL) {
            response = true;
        }
    }
    return response;
}

/*!
 * @brief function check_value_types checks if all fields in a record list belong to a table (through its definition)
 * AND if the corresponding value can be converted to the field type as specified in the table definition.
 * Uses find_field_definition and is_value_valid
 * @see find_field_definition
 * @see is_value_valid
 * @param fields_list list of the fields and their values (as text)
 * @param table_definition table definition
 * @return true if all fields belong to table and their value types are correct, false else
 */
bool check_value_types(table_record_t *fields_list, table_definition_t *table_definition) {
    bool response = true;
    field_definition_t *field_definition;

    for (int i=0; i<fields_list->fields_count; i++) { //Pour chaque champ de la liste
        field_definition = find_field_definition(&fields_list->fields[i].column_name, table_definition);
        //printf("check_value_types-Type: %d - %s\n", field_definition->column_type, field_definition->column_name);
        if (field_definition == NULL) {
            response = false;
        } else if (!is_value_valid(&fields_list->fields[i], field_definition)) {
            response = false;
        }
    }
    return response;
}

/*!
 * @brief function find_field_definition looks up for a field name in a table definition.
 * @param field_name the field name to lookup for.
 * @param table_definition the table definition in which to search.
 * @return a pointer to the field definition structure if the field name exists, NULL if it doesn't.
 */
field_definition_t *find_field_definition(char *field_name, table_definition_t *table_definition) {
    bool exist_temp = true;
    for (int i=0; i<table_definition->fields_count; i++) { //On parcours chaque champs de table_definition
        exist_temp = true;
        if (strlen(field_name) == strlen(table_definition->definitions[i].column_name)) { //Si le champs fait la meme longueur que field_name
            for (int j=0; (j<strlen(field_name) && (j<strlen(table_definition->definitions[i].column_name))); j++) { //On parcours chaque lettre 
                if (field_name[j] != table_definition->definitions[i].column_name[j]) { //Si une lettre est diffférente
                    exist_temp=false; //Les mots sont différents
                }
            }
        }
        else { //Si les champs ne font pas la même longueur, ils sont différents
            exist_temp = false;
        }
    
        if (exist_temp) { //Si les champs correspondent
            return &table_definition->definitions[i]; // On retourne la valeur de field_definition
        }
    }
    return NULL;
}

/*!
 * @brief function is_value_valid checks if a field_record_t holding a field name and a text value matches a field definition:
 * it checks fields names, and tests if the text value can be converted into the table defined type.
 * /!\ If it succeeded, this function must modify the value in the field_record_t structure by converting and setting
 * the value to the proper type!
 * Uses is_int, is_float, is_key
 * @see is_int
 * @see is_float
 * @see is_key
 * @param value the field and value record
 * @param field_definition the field definition to test against
 * @return true if valid (and converted), false if invalid
 */
bool is_value_valid(field_record_t *value, field_definition_t *field_definition) {
    char *eptr;
    bool response = false;

    if (strcmp(value->column_name, field_definition->column_name) == 0) {
        switch(field_definition->column_type) {
            case TYPE_INTEGER:
                if (is_int(value->field_value.text_value)) {
                    value->field_type = TYPE_INTEGER;
                    value->field_value.int_value = strtoll(value->field_value.text_value, &eptr, 10); //Pas d'erreur car déjà testé dans is_int
                    response = true;
                }
                break;
            case TYPE_FLOAT:
                if (is_float(value->field_value.text_value)) {
                    value->field_type = TYPE_FLOAT;
                    value->field_value.float_value = strtod(value->field_value.text_value, &eptr); //Pas d'erreur car déjà testé dans is_float
                    response = true;
                }
                break;
            case TYPE_PRIMARY_KEY:
                if (is_key(value->field_value.text_value)) {
                    value->field_type = TYPE_PRIMARY_KEY;
                    value->field_value.primary_key_value = strtoull(value->field_value.text_value, &eptr, 10); //Pas d'erreur car déjà testé dans is_key
                    response = true;
                }
                break;
            case TYPE_TEXT:
                value->field_type = TYPE_TEXT;
                response = true;
                break;
            default:
                value->field_type = TYPE_UNKNOWN;
                response = false;
                break;
        }
    } 

    return response;
}

/*!
 * @brief function is_int tests if the string value is a text representation of an integer value.
 * You may use strtoll for this test.
 * @param value the text representation to test
 * @return true if value can be converted into an integer, false if it cannot
 */
bool is_int(char *value) {
    char *eptr;                             // First caracter non-converted by strtoll
    long long result; 

    result = strtoll(value, &eptr, 10);    // Convert to a decimal long long

    if (*eptr == '\0') {             // The entire string has been converted
        return true;
    } 

    return false;
}

/*!
 * @brief function is_float tests if the string value is a text representation of a double value.
 * You may use strtod for this test.
 * @param value the text representation to test
 * @return true if value can be converted into a double, false if it cannot
 */
bool is_float(char *value) {
    char *eptr;
    double result;

    result = strtod(value, &eptr);     // Convert to a double
    if (*eptr == '\0') {         // The entire string has been converted
        return true;
    } 

    return false;
}

/*!
 * @brief function is_key tests if the string value is a text representation of a key value.
 * You may use strtoull for this test.
 * @param value the text representation to test
 * @return true if value can be converted into a key, false if it cannot
 */
bool is_key(char *value) {
    char *eptr;
    unsigned long long result;

    result = strtoull(value, &eptr, 10);   // Convert to a decimal unsigned long         

    if (*eptr == '\0') {             // The entire string has been converted
        return true;
    } 

    return false;
}
