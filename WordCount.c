
/**********************************************************************************************************************/
/**
 **
 ** @file       WordCount.c
 ** @brief      Program to compute word count from a character stream.
 ** @date       August 14, 2018
 ** @version    1.0
 **
 **/
/**********************************************************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define MAXSIZE     50
#define NUM_THREADS 10

pthread_mutex_t mymutex = PTHREAD_MUTEX_INITIALIZER;

/* A structure for the words */
struct wordarray{
    char word[100];
    int frequency;
};

typedef struct wordarray WordArray;

enum count_type {
    FAST = 1,
    SLOW = 2
};

/* Function Prototypes */
int compareWords(const void *f1, const void *f2);
void *insert_words();
void print_results(WordArray words[], int counter);
char read_next_charecter(WordArray words[], int counter, int size);
void process_words(WordArray words[], int counter, int size);
void *getNextCharacter(void *arg);
void die(char *s);

/* Character stream */
char * stream = "Then hear me, gracious sovereign, and you peers,\
That owe yourselves, your lives and services \
To this imperial throne. There is no bar \
To make against your highness' claim to France \
But this, which they produce from Pharamond,\
'In terram Salicam mulieres ne succedant:'\
'No woman shall succeed in Salique land:'\
Which Salique land the French unjustly gloze \
To be the realm of France, and Pharamond \
The founder of this law and female bar.\
Yet their own authors faithfully affirm \
That the land Salique is in Germany,\
Between the floods of Sala and of Elbe;\
Where Charles the Great, having subdued the Saxons,\
There left behind and settled certain French;\
Who, holding in disdain the German women \
For some dishonest manners of their life,\
Establish'd then this law; to wit, no female \
Should be inheritrix in Salique land:\
Which Salique, as I said, 'twixt Elbe and Sala,\
Is at this day in Germany call'd Meisen.\
Then doth it well appear that Salique law \
Was not devised for the realm of France: \
Nor did the French possess the Salique land \
Until four hundred one and twenty years \
After defunction of King Pharamond,\
Idly supposed the founder of this law;\
Who died within the year of our redemption \
Four hundred twenty-six; and Charles the Great \
Subdued the Saxons, and did seat the French \
Beyond the river Sala, in the year \
Eight hundred five. Besides, their writers say \
King Pepin, which deposed Childeric,\
Did, as heir general, being descended \
Of Blithild, which was daughter to King Clothair,\
Make claim and title to the crown of France.\
Hugh Capet also, who usurped the crown \
Of Charles the duke of Lorraine, sole heir male \
Of the true line and stock of Charles the Great,\
To find his title with some shows of truth,\
'Through, in pure truth, it was corrupt and naught,\
Convey'd himself as heir to the Lady Lingare,\
Daughter to Charlemain, who was the son \
To Lewis the emperor, and Lewis the son \
Of Charles the Great. Also King Lewis the Tenth,\
Who was sole heir to the usurper Capet,\
Could not keep quiet in his conscience,\
Wearing the crown of France, till satisfied \
That fair Queen Isabel, his grandmother,\
Was lineal of the Lady Ermengare,\
Daughter to Charles the foresaid duke of Lorraine:\
By the which marriage the line of Charles the Great \
Was re-united to the crown of France.\
So that, as clear as is the summer's sun.\
King Pepin's title and Hugh Capet's claim,\
King Lewis his satisfaction, all appear \
To hold in right and title of the female:\
So do the kings of France unto this day;\
Howbeit they would hold up this Salique law \
To bar your highness claiming from the female,\
And rather choose to hide them in a net \
Than amply to imbar their crooked titles \
Usurp'd from you and your progenitors";

static int shmpos = 0;   /* Position reference for shared memory */
int word_count_type = FAST; /* Type of word count(fast or slow) */
int shmid; /* shared memory Id */
/*
 * Main function
 */
void main(int argc, char *argv[]) 
{

    int thread_count = 2; /* In case of default fast word count only 2 threads */
    int thread_id;
    key_t key = 5678;

    printf("Chooses fast or slow words count methods:\n 1. Fast \t 2. Slow\n");
    scanf("%d", &word_count_type);

    if(word_count_type == SLOW)
        thread_count = 11; /* 10 to read data one to process data*/

    pthread_t tid[NUM_THREADS];
    srand(time(NULL));

    if ((shmid = shmget(key, MAXSIZE, 0666)) < 0)
        die("shmget");


    for (thread_id = 0; thread_id < thread_count-1; thread_id++ )
    {
        pthread_create(&tid[thread_id], NULL , getNextCharacter, NULL);
    }

    /* Thread that runs function which converts characters in stream into words and store
     * in a data structure */
    pthread_create (&tid[thread_count-1], NULL ,insert_words, NULL);

    for (thread_id = 0; thread_id < thread_count; thread_id++ ) {
        pthread_join(tid[thread_id], NULL);
    }

    exit(0);
}

/*
 * Function used to form words out of a character stream and insert into a structure.
 */
void *insert_words()
{
    char buff[50];
    int pos = 0;
    int isUnique;
    int counter = 0;
    char c;

    WordArray words[1000];

    /* Scan the stream one character at a time */
    while ((c = (char)read_next_charecter(words, counter, sizeof(WordArray)))!= '\0')
    {
        /* form words from stream */
        switch (c)
        {
                /* Check for occurrence of delimiters in character stream and split into words */

                case ' ':
                case ':':
                case ',':
                case '.':
                case '\n':
                case ';':
                        {
                            /* Ignore if we encounter multiple delimiters sequentially */
                            if (pos == 0)
                                continue;

                            /* Form a word when we encounter a delimiter */
                            buff[pos] = '\0';

                            /* Consider the special case of words ending in single quotes */
                            if (buff[pos-1] == '\'')
                                buff[pos-1] = '\0';

                            pos = 0;
                            isUnique = -1;

                            /* String comparison - to check if the word is already in the array */
                            int num;
                            
                            for (num = 0; num < counter; num++){
                                if (strcmp(words[num].word, buff) == 0){
                                    isUnique = num;
                                }
                            }

                            /* If the string is not in the array, put it in the array */
                            if (isUnique == -1){
                                    
                                strcpy(words[counter].word, buff);
                                words[counter].frequency = 1;
                                counter++;
                            }
                            
                            /* Else increase frequency of non-unique words */
                            else {
                            words[isUnique].frequency++;
                           }
                        }
                        break;

                case '\'': /* In cases starting with a single -quote at the start of the word - Ignore */
                          {
                              if (pos == 0)
                                  continue;
                          }
                case '#': /* Ignore this return value. It returned when no characters are available to process yet*/
                          break;

                default:
                       {
                        /* Form words from Character Stream and increment counters */
                           if(isupper(c))
                               buff[pos++] = tolower(c);
                           else
                               buff[pos++] = c;
                       }
                       break;
                    
        } /* End of switch case */


    } /* End of while loop */

    /* For the last word */
    if(c == '\0')
    {

          if (pos != 0)
          {
              /* Form a word the last one*/
              buff[pos] = '\0';

              /* Consider the special case of words ending in single quotes */
              if (buff[pos-1] == '\'')
                  buff[pos-1] = '\0';

              isUnique = -1;

              /* String comparison - to check if the word is already in the array */
              int num;

              for (num = 0; num < counter; num++){
                   if (strcmp(words[num].word, buff) == 0){
                       isUnique = num;
                   }
              }

              /* If the string is not in the array, put it in the array */
              if (isUnique == -1){

                   strcpy(words[counter].word, buff);
                   words[counter].frequency = 1;
                   counter++;
              }

              /* Else increase frequency of non-unique words */
              else {
                   words[isUnique].frequency++;
              }

          } /* End of inner if */
     } /* End of outer if */

    /* Sort the words and print them in decreasing order of frequency */
    process_words(words, counter, sizeof(WordArray));

}

/*
 * Function used to compare words, to be used in qsort function
 */
int compareWords(const void *f1, const void *f2)
{

    WordArray *a = (WordArray *)f1;
    WordArray *b = (WordArray *)f2;

    if ( a->frequency != b->frequency)
        return (b->frequency > a->frequency);
    else {
        
        if (strcmp(a->word, b->word) > 0)
            return 1;
        else
            return 0; 
    }    

}

/*
 * Function to display words along with frequency 
 */
void print_results(WordArray words[], int counter ) 
{

    int num_words;

    for (num_words = 0; num_words < counter; num_words++){
        printf("%s -  %d\n", words[num_words].word, words[num_words].frequency);
    }

    printf("\nNo of Unique words :%d\n",counter);

}

/*
 * Function that reads and returns next character from shared memory.
 */
char read_next_charecter(WordArray words[], int counter, int size)
{

    static int pos = 0;
    static int last_pos = 0; /*variable to record last read of shared memory */
    //int shmid; /* shared memory Id */
    key_t key;
    char *shm, *s;
    key = 5678;
    int i;

        /* If more characters are inserted into shared memory */
        if(shmpos > last_pos)
        {

	    pthread_mutex_lock(&mymutex);

            if ((shm = shmat(shmid, NULL, 0)) == (char *) -1)
                die("shmat");

            //Now read what the threads put in the memory.
            s = shm ;
            s = s + last_pos;
            last_pos++;

            /* Release shared memory after reading final character */
            if(*s == '\0')
                shmctl(shmid, IPC_RMID, NULL);
	    pthread_mutex_unlock(&mymutex);
            return *s;

         } else {

             if (word_count_type == SLOW) {
                 /* If no characters are there to read during slow word count process the words and print the frequency */
                 process_words(words, counter, size);
			
                 /* Sleep */
                 sleep(1);
             }

             return '#';
         }
}

/*
 * Function to get next character of stream.
 */
void *getNextCharacter(void *arg)
{

    int sleeptime = rand()%10;
//    int shmid;
    key_t key;
    char *shm, *s;

    key = 5678;

    /* Read also null character as it is needed for loop termination in the insert_word function */
    while (shmpos < strlen(stream) + 1) {
        /* Apply mutex lock */
        pthread_mutex_lock(&mymutex);

        /* Attach shared memory */
        if ((shm = shmat(shmid, NULL, 0)) == (char *) -1)
            die("shmat");

        /* Store characters into shared memory */
        s = shm;
        s = s + shmpos;
        *s++ = stream[shmpos++];

        /* Release lock */
        pthread_mutex_unlock(&mymutex);

        /* If the type of word count is slow then put thread to sleep for random time */
        if (word_count_type == SLOW)
            sleep(sleeptime);
    }
}

/*
 * Function to initiate sort and print words of stream.
 */
void process_words(WordArray words[], int counter, int size)
{
    /* Sort the words to find most frequent words */
    qsort(words, counter, sizeof(WordArray), compareWords);

    /* Display words along with frequency */
    print_results(words, counter);
}

/*
 * Function to output errors.
 */
void die(char *s)
{
    perror(s);
    exit(1);
}
