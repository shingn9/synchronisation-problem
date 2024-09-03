#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// 4 cond var
// 2 mutex

// dont have one cond var for each group 
// teacher waits for students to arrive 
// students tell techer thier id 

// cond
/// 1. for making the teacher wait for all students to arrive
/// 2. for making students wait until the teacher is ready to give teacher their IDs
/// 3. one for making the teacher wait for students to finish repeating the IDs they just receied
/// 4. oen for making students wait for the teacher to tell them to leave


/// teacher thread calls broadcast 
// 1. each student need to wait for the teacher to tell them their id 
// 2. for teacher: to wait for the students to receive the next instructions once student told her the student id 
/// teacher wait for student to get ready 
/// teacher to dismiss students (students wait for teacher to dismiss students) -> pthread_broadcast (used in 2 places)
// ///// teacher thread calls broadcast 


// mutex
// one to control students leaving and entering 
// another one : to control student and techer for saying / listening student IDs
///////// to manage the distribution of IDs


void * teacher_routine(void *);
void * student_routine(void *);


int no_of_students; // each has unique ID
int no_of_groups;
int students_arrived = 0;
int students_left = 0;
int student_id_assigned = 0;
int students_repeated_ids = 0;
int * group_ids;

int curr_id_to_repeat = 0;


pthread_cond_t waiting_students_arrival; // teacher waits for all students to arrive
pthread_cond_t waiting_teacher_to_give_IDs; // students wait for teacher to assign group
pthread_cond_t waiting_repeating_ids; // teacher waits for students to repeat IDs
pthread_cond_t waiting_students_leave;  // for sutdents to hear from teacehr that they can leave now
// students wait for teacher's signal to leave

pthread_cond_t students_all_left; // teacher need confirmation that all students have left 


pthread_mutex_t students_mutex;
pthread_mutex_t distribute_IDs_mutex;

pthread_t *s_thrd_ids; // system thread id
pthread_t t_thrd_id; //system thread id

typedef struct student_object {
    int id;
	int group_id;
} student_obj;


int main(int argc, char ** argv) {

    // only one teacher
    student_obj *student; //user-defined thread id
 	int k, rc;


    // ask for total num of students in class
    printf("Enter the total number of students in class (int): ");
	scanf("%d", &no_of_students);
        printf("\n");
	
	//ask for the number of groups 
	printf("Enter the number of groups (int): ");
	scanf("%d", &no_of_groups);
        printf("\n\n");
	
    s_thrd_ids = malloc((no_of_students) * sizeof(pthread_t)); //student thread ids
	if(s_thrd_ids == NULL){
		fprintf(stderr, "threads out of memory\n");
		exit(1);
	}	
	
	student = malloc((no_of_students) * sizeof(student_obj)); //total is no_of_student
	if(student == NULL){
		fprintf(stderr, "t out of memory\n");
		exit(1);
	}	

    group_ids = (int *)malloc(no_of_students * sizeof(int));
    if (group_ids == NULL) {
        fprintf(stderr, "Failed to allocate memory for group IDs\n");
        exit(EXIT_FAILURE);
    }


    //Initialize condition variable and mutex objects 
	pthread_mutex_init(&students_mutex, NULL);
	pthread_mutex_init(&distribute_IDs_mutex, NULL);

    rc = pthread_cond_init(&waiting_students_arrival, NULL);
	if (rc) {
		printf("ERROR; return code from pthread_cond_init() (waiting_students_arrival) is %d\n", rc);
		exit(-1);
	}
	rc = pthread_cond_init(&waiting_teacher_to_give_IDs, NULL);
	if (rc) {
		printf("ERROR; return code from pthread_cond_init() (waiting_teacher_to_give_IDs) is %d\n", rc);
		exit(-1);
	}
    rc = pthread_cond_init(&waiting_repeating_ids, NULL);
	if (rc) {
		printf("ERROR; return code from pthread_cond_init() (waiting_repeating_ids) is %d\n", rc);
		exit(-1);
	}

    rc = pthread_cond_init(&waiting_students_leave, NULL);
	if (rc) {
		printf("ERROR; return code from pthread_cond_init() (waiting_students_leave) is %d\n", rc);
		exit(-1);
	}


    //create the teacher thread.
	if (pthread_create(&t_thrd_id, NULL, teacher_routine, NULL)){
		printf("ERROR; return code from pthread_create() (boat) is %d\n", rc);
		exit(-1);
	}

    //create consumers according to the arrival rate
	int n_c;
    int s_c;
    n_c = s_c = 0;
    int id;


    for (int k = 0; k<no_of_students; k++) {
        student[k].id = k;
		rc = pthread_create(&s_thrd_ids[k], NULL, student_routine, (void *)&student[k]);
		if (rc) {
			printf("ERROR; return code from pthread_create() (consumer) is %d\n", rc);
			exit(-1);
		}
    }

    //join student threads
    for (int k = 0; k<no_of_students; k++) {
		pthread_join(s_thrd_ids[k], NULL);
    }


    // pthread_mutex_lock(&students_mutex);
    // pthread_cond_signal(&waiting_students_leave); // Signal the teacher that all students have left
    // pthread_mutex_unlock(&students_mutex);
    pthread_join(t_thrd_id, NULL); // Wait for the teacher thread to exit

    //After all student threads exited, terminate the teacher thread
    // pthread_cancel(t_thrd_id); 
			
	//deallocate allocated memory
	free(s_thrd_ids);
	free(student);
    free(group_ids);

	//destroy mutex and condition variable objects
    pthread_mutex_destroy(&students_mutex);
	pthread_mutex_destroy(&distribute_IDs_mutex);


	pthread_cond_destroy(&waiting_students_arrival);
	pthread_cond_destroy(&waiting_teacher_to_give_IDs);
	pthread_cond_destroy(&waiting_repeating_ids);
	pthread_cond_destroy(&waiting_students_leave);

    printf("Main thread: This the end of simulation.\n");

    exit(0);
}



/// students havet o wait for teacher to geive them iDs
// need students to repeat id  (teacher waiting while students repeat)

// students wait for teacher to tell them they can all leave 


void * teacher_routine(void * arg) {

    // wait for all students to arrrive
    pthread_mutex_lock(&students_mutex);
    printf("Teacher: I'm waiting for all students to arrive.\n");
    // pthread_cond_broadcast(&waiting_students_leave);

    // is this 'if' or 'while' -> works the same 
    while (students_arrived < no_of_students) {
        // while not all students have arrived yet, continue waiting 
        pthread_cond_wait(&waiting_students_arrival, &students_mutex);
    }

    // once all students arrived
    printf("Teacher: All students have arrived. I start to assign group ids to students.\n");
    pthread_mutex_unlock(&students_mutex);


    int group_size;
    int first_n_mod_m_groups;
    int remaining_groups;
    
    // 7 students, 3 groups 
    //   7 % 3 = 1
    //   3,2,2
    // teacher start to assign students 

    // teacher broadcast to all students: that teacher is ready to assign IDs
    pthread_cond_broadcast(&waiting_teacher_to_give_IDs); 

    for (int g = 0; g < no_of_groups; g++) { 
        first_n_mod_m_groups = no_of_students % no_of_groups;
        group_size = (no_of_students/no_of_groups) + (g < first_n_mod_m_groups ? 1:0);

        for (int i = 0; i < group_size; i++) {
            pthread_mutex_lock(&distribute_IDs_mutex);
            printf("Teacher: student [%d] is in group [%d].\n", student_id_assigned, g);
            group_ids[student_id_assigned] = g;
            curr_id_to_repeat = student_id_assigned;
            pthread_cond_broadcast(&waiting_repeating_ids);
            student_id_assigned++;

            pthread_cond_wait(&waiting_repeating_ids, &distribute_IDs_mutex);
            // curr_id_to_repeat = student_id_assigned;
            // after assigning each student, teacher waits for student to repeat id 
            pthread_mutex_unlock(&distribute_IDs_mutex);
        }
    }


    // after assigning all, signal the students in order 
    // for (int i =0; i < no_of_students; i++) {
    //     pthread_mutex_lock(&distribute_IDs_mutex);
    //     curr_id_to_repeat = i;
    //     pthread_cond_broadcast(&waiting_repeating_ids);
    //     pthread_cond_wait(&waiting_repeating_ids, &distribute_IDs_mutex);
    //     pthread_mutex_unlock(&distribute_IDs_mutex);
    // }

    printf("Teacher: Students can leave now.\n");

    while (students_left < no_of_students) {
        pthread_mutex_lock(&students_mutex);
        pthread_cond_broadcast(&waiting_students_leave);
        pthread_mutex_unlock(&students_mutex);
    }

    // pthread_cond_broadcast(&waiting_students_leave);
    // pthread_cond_wait(&waiting_students_leave, &students_mutex);

    // doesnt get printed 
    printf("Teacher: All students have left. I can go home now.\n");
    pthread_exit(EXIT_SUCCESS);
}


void * student_routine(void * arg) {
    
    student_obj * student = (student_obj*) arg;
    pthread_mutex_lock(&students_mutex);

    printf("Student [%d]: I have arrived and wait for being assigned to a group.\n", student->id);
    students_arrived++;
    if (students_arrived == no_of_students) {
        pthread_cond_signal(&waiting_students_arrival);
    }
    pthread_mutex_unlock(&students_mutex);


    // after all  students arrive 
    // student wait for teacher to finish assigning group ids
    pthread_mutex_lock(&distribute_IDs_mutex);
    pthread_cond_wait(&waiting_teacher_to_give_IDs, &distribute_IDs_mutex);
    pthread_mutex_unlock(&distribute_IDs_mutex);


    // for student to repeat the id right after the teacher assigns a student
    pthread_mutex_lock(&distribute_IDs_mutex);
    while (curr_id_to_repeat != student->id) {
        pthread_cond_wait(&waiting_repeating_ids, &distribute_IDs_mutex);
    }
    int group_id = group_ids[student->id];
    printf("Student [%d]: OK, I'm in group [%d] and waiting for my turn to enter a lab room.\n", student->id, group_id);
    pthread_cond_signal(&waiting_repeating_ids);
    pthread_mutex_unlock(&distribute_IDs_mutex);



    // for students to leave now
    pthread_mutex_lock(&students_mutex);
    //// but condition is already met before the thread reaches this line
    pthread_cond_wait(&waiting_students_leave, &students_mutex);
    students_left++;
    pthread_mutex_unlock(&students_mutex);
    
    printf("Student [%d]: Bye Teacher.\n", student->id);
    if (students_left == no_of_students) {
        pthread_cond_signal(&students_all_left);
    }
    pthread_exit(EXIT_SUCCESS);
}

