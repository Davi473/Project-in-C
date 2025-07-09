#include <stdio.h>
#include <string.h>
#include "../include/item.h"

Item add() 
{
    printf("\n\n\n-------------------------------------------------------------------------------\n");
    printf("-------------------------------------------------------------------------------\n");
    Item item = {};
    printf("Nome: ");
    scanf("%49s", item.nome);

    printf("Amount: ");
    scanf("%lf", &item.amount);

    printf("Quantity: ");
    scanf("%lf", &item.quantity);

    printf("Category: ");
    scanf("%29s", item.category);
    printf("-------------------------------------------------------------------------------\n");
    printf("-------------------------------------------------------------------------------\n");
    return item;
}

void print_item(const Item* item) {
    printf("| %-20s | %10.2f | %12.5f | %-15s |\n",
        item->nome, item->amount, item->quantity, item->category);
}

void print_lista(const Item* lista, int n) {
    printf("\n\n\n-------------------------------------------------------------------------------\n");
    printf("| %-20s | %10s | %12s | %-15s |\n", "Nome", "Amount", "Quantity", "Category");
    printf("-------------------------------------------------------------------------------\n");
    for (int i = 0; i < n; i++) {
        print_item(&lista[i]);
    }
    printf("-------------------------------------------------------------------------------\n");
}

int options() {
    printf("\n\n\n-------------------------------------------------------------------------------\n");
    printf("-------------------------------------------------------------------------------\n");
    int option;
    printf("1. Add Item\n");
    printf("2. Print List\n");
    printf("3. Exit\n");
    printf("Choose an option: ");
    scanf("%d", &option);
    printf("-------------------------------------------------------------------------------\n");
    printf("-------------------------------------------------------------------------------\n");
    return option;
}

void add_or_update_item(Item* lista, int* n, Item novoItem) {
    int found = 0;
    for (int i = 0; i < *n; i++) {
        if (strcmp(lista[i].nome, novoItem.nome) == 0) {
            double quantidade = lista[i].quantity + novoItem.quantity;
            double amount = (lista[i].amount * lista[i].quantity) + (novoItem.amount * novoItem.quantity);
            lista[i].amount = amount / quantidade;
            lista[i].quantity = quantidade;
            found = 1;
            break;
        }
    }
    if (!found && *n < TAM) {
        lista[*n] = novoItem;
        (*n)++;
    }
}
