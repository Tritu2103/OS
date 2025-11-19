#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#define TRUE 1
#define FALSE 0
#define DELAY	sleep(rand() % 4)

int account; // tai khoan ngan hang
int turn;
int interest[2];

enter_region(int pid) {
	int other;
	 other = 1 - pid;
    interest[pid] = TRUE;
    turn = pid;
    while (interest[other] == TRUE && turn == pid);
}

leave_region(int pid) {
	interest[pid] = FALSE;

}

void *chaGuiTien() {  // luot =0
	int x;
	while (TRUE) {
		enter_region(0); 
		int amount = rand() % 100 + 1; DELAY; // Số tiền gửi ngẫu nhiên 
        account += amount;
        printf("Bo: Da gui %d. Account=%d\n", amount, account); DELAY;
		leave_region(0);
		printf("Bo: Tao da gui. Account=%d\n", account);DELAY;
	}
}

void *conRutTien() { // luot =1
	int y;
	while (TRUE) {
		enter_region(1); 
		int amount = rand() % 50 + 1; DELAY; // Số tiền rút ngẫu nhiên
        if (account >= amount) {
            account -= amount;
            printf("Con: Da rut %d. Account=%d\n", amount, account); DELAY;
        } else {
            printf("Con: Khong du tien de rut %d. Account=%d\n", amount, account); DELAY;
        }
		leave_region(1); 
		DELAY;
		
	}
}

void main() {
   pthread_t tid1, tid2;
   
   printf("Account=%d\n",account);
   pthread_create(&tid1, NULL, chaGuiTien, NULL);   
   pthread_create(&tid2, NULL, conRutTien, NULL);   
   
   pthread_join(tid1, NULL);  
   pthread_join(tid2, NULL);  
}