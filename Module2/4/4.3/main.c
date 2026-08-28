#include "contacts.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void safe_get_line(const char *prompt, char *buffer, size_t size) {
    printf("%s", prompt);
    if (fgets(buffer, size, stdin)) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        } else {
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
        }
    }
}

int get_int_option(const char *prompt) {
    char buffer[32];
    safe_get_line(prompt, buffer, sizeof(buffer));
    char *endptr;
    long val = strtol(buffer, &endptr, 10);
    if (endptr == buffer || *endptr != '\0') {
        return -1;
    }
    return (int)val;
}

void handle_list_contacts(const contacts_t *book) {
    size_t count = contacts_get_count(book);
    printf("\n=== CONTACT LIST (%zu contacts) ===\n", count);
    if (count == 0) {
        printf("[No contacts stored yet]\n");
        return;
    }
    for (size_t i = 0; i < count; i++) {
        const record_t *rec = contacts_get_at(book, i);
        printf("[%zu] %s %s\n", i + 1, rec->first_name.data, rec->last_name.data);
    }
}

void handle_view_contact(const contacts_t *book) {
    size_t count = contacts_get_count(book);
    if (count == 0) {
        printf("\nNo contacts to view.\n");
        return;
    }

    handle_list_contacts(book);
    int index = get_int_option("\nEnter contact index to view: ");
    if (index < 1 || (size_t)index > count) {
        printf("Error: Invalid contact index.\n");
        return;
    }

    const record_t *rec = contacts_get_at(book, (size_t)index - 1);
    printf("\n=== CONTACT DETAILS ===\n");
    printf("First Name:    %s\n", rec->first_name.data);
    printf("Last Name:     %s\n", rec->last_name.data);
    printf("Workplace:     %s\n", rec->place_of_work.data ? rec->place_of_work.data : "[Not Specified]");
    printf("Job Position:  %s\n", rec->position.data ? rec->position.data : "[Not Specified]");

    printf("\n--- Phone Numbers ---\n");
    if (rec->numbers.size == 0) printf("[None]\n");
    for (size_t i = 0; i < rec->numbers.size; i++) {
        printf("  [%zu] %s\n", i + 1, rec->numbers.items[i].data);
    }

    printf("\n--- Email Addresses ---\n");
    if (rec->emails.size == 0) printf("[None]\n");
    for (size_t i = 0; i < rec->emails.size; i++) {
        printf("  [%zu] %s\n", i + 1, rec->emails.items[i].data);
    }

    printf("\n--- Social Networks ---\n");
    if (rec->socials.size == 0) printf("[None]\n");
    for (size_t i = 0; i < rec->socials.size; i++) {
        printf("  %s: %s\n", rec->socials.items[i].key.data, rec->socials.items[i].value.data);
    }

    printf("\n--- Messengers ---\n");
    if (rec->messengers.size == 0) printf("[None]\n");
    for (size_t i = 0; i < rec->messengers.size; i++) {
        printf("  %s: %s\n", rec->messengers.items[i].key.data, rec->messengers.items[i].value.data);
    }
    printf("=======================\n");
}

void handle_add_contact(contacts_t *book) {
    char first_name[128] = {0};
    char last_name[128] = {0};
    char workplace[128] = {0};
    char position[128] = {0};

    printf("\n=== ADD NEW CONTACT ===\n");
    safe_get_line("Enter First Name (Required): ", first_name, sizeof(first_name));
    safe_get_line("Enter Last Name (Required): ", last_name, sizeof(last_name));
    safe_get_line("Enter Workplace (Optional, Press Enter to skip): ", workplace, sizeof(workplace));
    safe_get_line("Enter Position (Optional, Press Enter to skip): ", position, sizeof(position));

    record_t temp;
    record_init(&temp);

    contacts_status_t status = record_set_first_name(&temp, first_name);
    if (status != CONTACTS_OK) {
        printf("Error: First Name is required and cannot be empty.\n");
        record_free(&temp);
        return;
    }

    status = record_set_last_name(&temp, last_name);
    if (status != CONTACTS_OK) {
        printf("Error: Last Name is required and cannot be empty.\n");
        record_free(&temp);
        return;
    }

    if (strlen(workplace) > 0) record_set_place_of_work(&temp, workplace);
    if (strlen(position) > 0) record_set_position(&temp, position);

    char add_more[8];
    safe_get_line("\nDo you want to add a phone number now? (y/n): ", add_more, sizeof(add_more));
    if (strcmp(add_more, "y") == 0 || strcmp(add_more, "Y") == 0) {
        char phone[64];
        safe_get_line("Enter phone number: ", phone, sizeof(phone));
        record_add_number(&temp, phone);
    }

    status = contacts_add(book, &temp);
    if (status == CONTACTS_OK) {
        printf("\nContact added successfully!\n");
    } else {
        printf("\nError: Failed to save the contact.\n");
    }
    record_free(&temp);
}

void manage_numbers(record_t *rec) {
    while (1) {
        printf("\n--- Manage Phone Numbers ---\n");
        for (size_t i = 0; i < rec->numbers.size; i++) {
            printf("  [%zu] %s\n", i + 1, rec->numbers.items[i].data);
        }
        printf("1. Add Number\n");
        printf("2. Edit Number\n");
        printf("3. Delete Number\n");
        printf("4. Back to Edit Menu\n");
        
        int choice = get_int_option("Choose action: ");
        if (choice == 4) break;

        char buffer[128];
        if (choice == 1) {
            safe_get_line("Enter phone number: ", buffer, sizeof(buffer));
            record_add_number(rec, buffer);
        } else if (choice == 2 || choice == 3) {
            int idx = get_int_option("Enter list index: ");
            if (idx < 1 || (size_t)idx > rec->numbers.size) {
                printf("Error: Invalid index.\n");
                continue;
            }
            if (choice == 2) {
                safe_get_line("Enter new phone number: ", buffer, sizeof(buffer));
                record_update_number_at(rec, (size_t)idx - 1, buffer);
            } else {
                record_delete_number_at(rec, (size_t)idx - 1);
            }
        }
    }
}

void manage_socials(record_t *rec) {
    while (1) {
        printf("\n--- Manage Social Networks ---\n");
        for (size_t i = 0; i < rec->socials.size; i++) {
            printf("  [%zu] %s: %s\n", i + 1, rec->socials.items[i].key.data, rec->socials.items[i].value.data);
        }
        printf("1. Add Social Link\n");
        printf("2. Edit Social Link\n");
        printf("3. Delete Social Link\n");
        printf("4. Back to Edit Menu\n");

        int choice = get_int_option("Choose action: ");
        if (choice == 4) break;

        char name[128], link[256];
        if (choice == 1) {
            safe_get_line("Enter platform name (e.g., GitHub): ", name, sizeof(name));
            safe_get_line("Enter profile URL: ", link, sizeof(link));
            record_add_social(rec, name, link);
        } else if (choice == 2 || choice == 3) {
            int idx = get_int_option("Enter index: ");
            if (idx < 1 || (size_t)idx > rec->socials.size) {
                printf("Error: Invalid index.\n");
                continue;
            }
            if (choice == 2) {
                safe_get_line("Enter new platform name: ", name, sizeof(name));
                safe_get_line("Enter new profile URL: ", link, sizeof(link));
                record_update_social_at(rec, (size_t)idx - 1, name, link);
            } else {
                record_delete_social_at(rec, (size_t)idx - 1);
            }
        }
    }
}

void handle_edit_contact(contacts_t *book) {
    size_t count = contacts_get_count(book);
    if (count == 0) {
        printf("\nNo contacts to edit.\n");
        return;
    }

    handle_list_contacts(book);
    int index = get_int_option("\nEnter contact index to edit: ");
    if (index < 1 || (size_t)index > count) {
        printf("Error: Invalid index.\n");
        return;
    }

    record_t *rec = contacts_get_mut_at(book, (size_t)index - 1);

    while (1) {
        printf("\n=== EDIT CONTACT: %s %s ===\n", rec->first_name.data, rec->last_name.data);
        printf("1. Edit First Name\n");
        printf("2. Edit Last Name\n");
        printf("3. Edit Workplace\n");
        printf("4. Edit Position\n");
        printf("5. Manage Phone Numbers\n");
        printf("6. Manage Social Links\n");
        printf("7. Back to Main Menu\n");

        int choice = get_int_option("Choose field to update (1-7): ");
        if (choice == 7) break;

        char buffer[256];
        contacts_status_t status;

        switch (choice) {
            case 1:
                safe_get_line("Enter new First Name: ", buffer, sizeof(buffer));
                status = record_set_first_name(rec, buffer);
                if (status != CONTACTS_OK) printf("Error: First Name cannot be empty.\n");
                break;
            case 2:
                safe_get_line("Enter new Last Name: ", buffer, sizeof(buffer));
                status = record_set_last_name(rec, buffer);
                if (status != CONTACTS_OK) printf("Error: Last Name cannot be empty.\n");
                break;
            case 3:
                safe_get_line("Enter new Workplace: ", buffer, sizeof(buffer));
                record_set_place_of_work(rec, buffer);
                break;
            case 4:
                safe_get_line("Enter new Position: ", buffer, sizeof(buffer));
                record_set_position(rec, buffer);
                break;
            case 5:
                manage_numbers(rec);
                break;
            case 6:
                manage_socials(rec);
                break;
            default:
                printf("Error: Unknown option.\n");
                break;
        }
    }
}

void handle_delete_contact(contacts_t *book) {
    size_t count = contacts_get_count(book);
    if (count == 0) {
        printf("\nNo contacts to delete.\n");
        return;
    }

    handle_list_contacts(book);
    int index = get_int_option("\nEnter index of contact to delete: ");
    if (index < 1 || (size_t)index > count) {
        printf("Error: Invalid index.\n");
        return;
    }

    contacts_status_t status = contacts_delete_at(book, (size_t)index - 1);
    if (status == CONTACTS_OK) {
        printf("Contact deleted successfully.\n");
    } else {
        printf("Error: Failed to delete contact.\n");
    }
}

int main(void) {
    contacts_t book;
    if (contacts_init(&book) != CONTACTS_OK) {
        fprintf(stderr, "Fatal error: failed to initialize memory database.\n");
        return 1;
    }

    while (1) {
        printf("\n========================================\n");
        printf("             CONTACT BOOK               \n");
        printf("========================================\n");
        printf("1. List All Contacts\n");
        printf("2. View Contact Details\n");
        printf("3. Add New Contact\n");
        printf("4. Edit Existing Contact\n");
        printf("5. Delete Contact\n");
        printf("6. Print Tree Structure (Graphical)\n");
        printf("7. Exit\n");                           
        printf("========================================\n");

        int choice = get_int_option("Choose an option (1-7): ");
        if (choice == 7) {
            printf("\nExiting.\n");
            break;
        }

        switch (choice) {
            case 1:
                handle_list_contacts(&book);
                break;
            case 2:
                handle_view_contact(&book);
                break;
            case 3:
                handle_add_contact(&book);
                break;
            case 4:
                handle_edit_contact(&book);
                break;
            case 5:
                handle_delete_contact(&book);
                break;
            case 6:
                contacts_print_tree(&book);
                break;
            default:
                printf("Invalid selection. Please try again.\n");
                break;
        }
    }

    contacts_free(&book);
    return 0;
}