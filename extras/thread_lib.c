#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

// Readers, librarians, and books
const char *readers[] = {"Alice", "Bob", "Claire", "Dorothy", "Emily"};
const char *librarians[] = {"Farouk", "Gizem"};
const int book_count = 3;
const int reader_count = 5;
const int librarian_count = 2;

// Book status: 0 = available, 1 = borrowed
int books[3] = {0, 0, 0};

// Synchronization primitives
pthread_mutex_t book_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t request_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t request_cond = PTHREAD_COND_INITIALIZER;

typedef struct {
    int reader_id;
    int book_id;
    int request_pending;
    int book_granted;
} BookRequest;

BookRequest current_request = {-1, -1, 0, 0};

// function to get random number
int get_random(int max) {
    return rand() % max;
}

// Reader thread based on given approach 
void *reader_thread(void *arg) {
    int id = *(int *)arg;
    const char *name = readers[id];
    int books_to_read = 2;

    while (books_to_read > 0) { // there are books to read
        int desired_book = get_random(book_count);

        pthread_mutex_lock(&request_mutex);
        while (current_request.request_pending) {  
            pthread_mutex_unlock(&request_mutex);
            usleep(10000); 
            pthread_mutex_lock(&request_mutex);
        }

        // attempt to borrow the book
        current_request.reader_id = id;
        current_request.book_id = desired_book;
        current_request.request_pending = 1;
        current_request.book_granted = 0;

        printf("%s wants to borrow Book %d\n", name, desired_book);
        pthread_cond_signal(&request_cond);
        pthread_mutex_unlock(&request_mutex);

        // Wait for librarian to process 
        while (1) {
            pthread_mutex_lock(&request_mutex);
            if (!current_request.request_pending && current_request.reader_id == id) {
                if (current_request.book_granted) { // If attempt is successful
                    // Read the book
                    pthread_mutex_unlock(&request_mutex);
                    printf("%s is reading Book %d \n", name, desired_book);
                    sleep(1); // Simulate reading time

                    // Return the book
                    pthread_mutex_lock(&book_mutex);
                    books[desired_book] = 0; // Returning book
                    pthread_mutex_unlock(&book_mutex);
                    printf("%s returned Book %d \n", name, desired_book);
                    books_to_read--;
                } else {
                    // go dancing
                    pthread_mutex_unlock(&request_mutex);
                    printf("%s goes dancing (Book %d is taken)\n", name, desired_book);
                    sleep(1); 
                }
                break;
            }
            pthread_mutex_unlock(&request_mutex);
            usleep(10000);
        }
    }

    printf("%s has finished reading books.\n", name);
    return NULL;
}

// Librarian thread based on given approach
void *librarian_thread(void *arg) {
    int id = *(int *)arg;
    const char *name = librarians[id];

    while (1) { 
        pthread_mutex_lock(&request_mutex);
        while (!current_request.request_pending) { // There are readers who may requests books 
            pthread_cond_wait(&request_cond, &request_mutex);  // Wait for a reader to make a request
        }

        int reader_id = current_request.reader_id;
        int book_id = current_request.book_id;

        pthread_mutex_lock(&book_mutex);
        if (books[book_id] == 0) { // If book is available
            books[book_id] = 1; // lend the book
            current_request.book_granted = 1;
            printf("%s lends Book %d to %s \n", name, book_id, readers[reader_id]);
        } else {
            // Send the reader away without the book
            current_request.book_granted = 0;
            printf("%s informs %s: Book %d is not available \n", name, readers[reader_id], book_id);
        }
        pthread_mutex_unlock(&book_mutex);

        current_request.request_pending = 0;
        pthread_mutex_unlock(&request_mutex);
        usleep(10000);
    }

    return NULL;
}

// Main
int main() {
    srand(time(NULL));

    pthread_t reader_threads[reader_count];
    pthread_t librarian_threads[librarian_count];
    int ids[reader_count > librarian_count ? reader_count : librarian_count];

    // Create librarian threads
    for (int i = 0; i < librarian_count; i++) {
        ids[i] = i;
        pthread_create(&librarian_threads[i], NULL, librarian_thread, &ids[i]);
    }

    // Create reader threads
    for (int i = 0; i < reader_count; i++) {
        ids[i] = i;
        pthread_create(&reader_threads[i], NULL, reader_thread, &ids[i]);
    }

    // Wait for readers to finish
    for (int i = 0; i < reader_count; i++) {
        pthread_join(reader_threads[i], NULL);
    }

    // Terminate program after readers are done (librarians loop forever)
    printf("\nAll readers are done. Closing library.\n");
    exit(0);
}