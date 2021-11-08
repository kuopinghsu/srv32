#include <stdio.h>
#include <stdlib.h>

int main(void) {
    FILE *fp;
    int i;

    if ((fp = fopen("out.bin", "wb")) == NULL) {
        printf("file create fail\n");
        return -1;
    }

    srand(10);
    for(i = 0; i < 4096; i++) {
        int res;
        int n = rand();
        res = fwrite((void*)&n, sizeof(int), 1, fp);
        if (res != 1) {
            printf("write fail\n");
            return -1;
        }
    }

    fclose(fp);

    if ((fp = fopen("out.bin", "rb")) == NULL) {
        printf("file read fail\n");
        return -1;
    }

    // get file size
    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    printf("file %d bytes\n", size);

    srand(10);
    for(i = 0; i < size / sizeof(int); i++) {
        int n;
        int res;
        res = fread(&n, sizeof(int), 1, fp);
        if (res != 1) {
            printf("read fail\n");
            return -1;
        }
        if (n != rand()) {
            printf("data compare failed (%d,%d)\n", i, n);
            return -1;
        }
    }


    fclose(fp);

    printf("Done\n");

    return 0;
}

