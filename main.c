
#include <stdio.h>
#include <locale.h>
#include "include/item.h"

int main()
{
    setlocale(LC_ALL, "Portuguese");

    Item lista[TAM] = {
        {"VALE", 8.80, 2.83933, "stock"},
        {"CIG", 1.88, 3.18302, "stock"},
        {"VOO", 542.33, 0.03687, "etf"}
    };
    int n = 3;
    int condition = 1;
    while (condition)
    {
           int option = options();

        switch (option)
        {
            case 1:
                Item novoItem = add();
                add_or_update_item(lista, &n, novoItem);
                break;
            case 2:
                print_lista(lista, n);
                break;
            case 3:
                condition = 0;
            default:
                break;
        }
    }
    return 0;
}