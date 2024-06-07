#include <stdio.h>
#include <sys/stat.h>

int main()
{
    char dir_name[100];

    printf("Enter directory name: ");
    scanf("%99s", dir_name);

    if (mkdir(dir_name, 0755) == 0)
    {
        printf("Directory '%s' created successfully.\n", dir_name);
    }
    else
    {
        perror("Error creating directory");
    }

    return 0;
}