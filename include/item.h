#ifndef ITEM_H
#define ITEM_H

#define TAM 1000

typedef struct {
    char nome[50];
    double amount;
    double quantity;
    char category[30];
} Item;

Item add();
void print_item(const Item* item);
void print_lista(const Item* lista, int n);
void add_or_update_item(Item* lista, int* n, Item novoItem);
int options();

#endif // ITEM_H
