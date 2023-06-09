//
// Created by flassabe on 22/11/2021.
//

#include "query_exec.h"

#include <dirent.h>
#include <unistd.h>

#include "database.h"
#include "table.h"

void execute(query_result_t *query) {
    switch (query->query_type) {
        case QUERY_CREATE_TABLE:
            execute_create(&query->query_content.create_query);
            break;
        case QUERY_INSERT:
            execute_insert(&query->query_content.insert_query);
            break;
        case QUERY_SELECT:
            execute_select(&query->query_content.select_query);
            break;
        case QUERY_UPDATE:
            execute_update(&query->query_content.update_query);
            break;
        case QUERY_DELETE:
            execute_delete(&query->query_content.delete_query);
            break;
        case QUERY_DROP_TABLE:
            execute_drop_table(query->query_content.table_name);
            break;
        case QUERY_DROP_DB:
            execute_drop_database(query->query_content.database_name);
            break;
        default:
            break; 
    }
}

void execute_create(create_query_t *query) {
    char buffer[300];
    char *chemin;
    char *parent;
    chemin = getcwd(buffer, sizeof(buffer)); //On récupère le répertoire courant
    parent = strstr(chemin+strlen(chemin)-4, "/db"); //On test si le dernier répertoire est "db"
    if ((parent == NULL) || (strcmp(parent, "/db") != 0)) { //Si on est pas dans "db"
    
        if (!directory_exists("db")) {
            create_db_directory("db");
        }
        chdir("db");
    }
    create_table(query);
}

void execute_insert(insert_query_t *query) {
    add_row_to_table(query->table_name, &query->fields_names);
}

void execute_select(update_or_select_query_t *query) {
    record_list_t *record_list = malloc(sizeof(record_list_t));
    clear_list(record_list);
    get_filtered_records(query->table_name, &query->set_clause, &query->where_clause, record_list);
    display_table_record_list(record_list);
    clear_list(record_list);
    free(record_list);
}

void execute_update(update_or_select_query_t *query) {
    set_row_to_table(query->table_name, &query->set_clause, &query->where_clause);    
}

void execute_delete(delete_query_t *query) {
    delete_row_to_table(query->table_name, &query->where_clause);
}

void execute_drop_table(char *table_name) {
    drop_table(table_name);
}

void execute_drop_database(char *db_name) {
    char buffer[TEXT_LENGTH];
    char *chemin;
    char *parent;
    chemin = getcwd(buffer, TEXT_LENGTH); //On récupère le répertoire courant
    parent = strstr(chemin+strlen(chemin)-4, "/db"); //On test si le dernier répertoire est "db"
    if ((parent != NULL) && (strcmp(parent, "/db") == 0)) {
        chdir(".."); //Si on est dans le répertoire "db" on revient en arrière
    }
    recursive_rmdir(db_name);
}
