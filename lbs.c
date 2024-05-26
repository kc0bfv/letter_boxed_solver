// Copyright [2024] <Karl Sickendick kc0bfv@gmail.com>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define DEFAULT_DICT "scrabble_dictionary.txt"

#define LETTER_IN_BUFFER_LEN 12 + 1
#define LETT_TO_IND(l) (l - 'a')
#define COUNTOF(arr) (sizeof(arr) / sizeof(arr[0]))
#define INIT_STR "THIS IS THE INITIAL VALUE FOR THE SHORTEST "\
    "SOLUTION EVERYTHING SHOULD BE SHORTER"

/* Convert input to lowercase, or return 0 if it's not a valid ascii letter
*/
char tolower_ascii(char input) {
    if ( input >= 'a' && input <= 'z' ) {
        return input;
    }
    if ( input >= 'A' && input <= 'Z' ) {
        return input - 'A' + 'a';
    }

    // else - invalid input
    return 0;
}

// Returns 0 on error, or the num of chars lowered
int tolower_ascii_str(char * input, size_t len) {
    for ( int i = 0; i < len; i++ ) {
        char lowered = tolower_ascii(input[i]);
        if ( 0 == lowered ) {
            return 0;
        }
        input[i] = lowered;
    }
    return len;
}
typedef struct {
    char prime;
    char nexts[9];
} NextChars;

typedef struct {
    NextChars * letters[26];
} LetterBoxed;

LetterBoxed * allocate_box() {
    LetterBoxed * retval = malloc(sizeof(LetterBoxed));
    if ( NULL == retval ) {
        perror("Error allocating LetterBoxed");
        return NULL;
    }
    for ( int i = 0; i < COUNTOF(retval->letters); i++ ) {
        retval->letters[i] = NULL;
    }
    return retval;
}

void destroy_box(LetterBoxed * box) {
    for ( int i = 0; i < COUNTOF(box->letters); i++ ) {
        if ( 0 != box->letters[i] ) {
            free(box->letters[i]);
        }
    }
    free(box);
}

LetterBoxed * create_box(char line[LETTER_IN_BUFFER_LEN]) {
    char lower_line[LETTER_IN_BUFFER_LEN] = "";
    for ( int i = 0; i < LETTER_IN_BUFFER_LEN - 1; i++ ) {
        char lett = tolower_ascii(line[i]);
        if ( 0 == lett ) {
            fprintf(stderr, "Error converting character: %i", line[i]);
            return NULL;
        }
        lower_line[i] = lett;
    }

    LetterBoxed * retval = allocate_box();
    if ( NULL == retval ) {
        return NULL;
    }

    for ( int i = 0; i < COUNTOF(lower_line) - 1; i++ ) {
        char lett = lower_line[i];
        if ( NULL != retval->letters[LETT_TO_IND(lett)] ) {
            fprintf(stderr, "Duplicate letter: %c\n", lett);
            destroy_box(retval);
            return NULL;
        }

        NextChars * nc = calloc(1, sizeof(NextChars));
        if ( NULL == nc ) {
            perror("Error allocating NextChars");
            destroy_box(retval);
            return NULL;
        }

        nc->prime = lett;
        // Build the list of next possible letters
        for ( int l = 0, next_loc = 0; l < COUNTOF(lower_line) - 1; l++ ) {
            // Only take letters that aren't on the line with the current
            // If i is 0-2, only take 3-11. If i is 3-5, only take 0-2 and 6-11,
            // etc.
            if ( l / 3 == i / 3 ) {
                continue;
            }

            // Write this letter into the nexts, and increment the next position
            if ( next_loc >= COUNTOF(nc->nexts) ) {
                fprintf(stderr, "Attempting to write too many nexts\n");
                destroy_box(retval);
                return NULL;
            }
            nc->nexts[next_loc] = lower_line[l];
            next_loc += 1;
        }
        retval->letters[LETT_TO_IND(lett)] = nc;
    }

    return retval;
}

bool is_word_in_box(LetterBoxed * box, char * word, size_t word_len) {
    if ( NULL == box || NULL == word || 0 == word_len ) {
        fprintf(stderr, "Invalid condition for is_word_in_box\n");
        return false;
    }

    NextChars * prev_letter = box->letters[LETT_TO_IND(word[0])];
    if ( NULL == prev_letter ) {
        return false;
    }
    if ( prev_letter->prime != word[0] ) {
        fprintf(stderr, "Invalid prime letter: %c\n", word[0]);
        return false;
    }

    for ( int i = 1; i < word_len; i++ ) {
        NextChars * new_prev = NULL;
        char lett = word[i];
        for ( int j = 0; j < COUNTOF(prev_letter->nexts); j++ ) {
            if ( prev_letter->nexts[j] == lett ) {
                new_prev = box->letters[LETT_TO_IND(lett)];
                if ( lett != new_prev->prime ) {
                    fprintf(stderr, "Invalid prime letter: %c\n", lett);
                    return false;
                }
                break;
            }
        }
        if ( NULL == new_prev ) {
            return false;
        }
        prev_letter = new_prev;
    }
    return true;
}

typedef struct WordNode {
    struct WordNode * next;
    char * word;
} WordNode;

typedef struct {
    WordNode * head;
    WordNode * tail;
} WordList;

typedef struct {
    WordList * letters[26];
} WordListByFirst;


WordList * create_list() {
    WordList * retval = malloc(sizeof(WordList));
    if ( NULL == retval ) {
        perror("Error allocating WordList");
        return NULL;
    }
    retval->head = NULL;
    retval->tail = NULL;
    return retval;
}

void destroy_list(WordList * list) {
    WordNode * cur = list->head;
    while ( NULL != cur ) {
        WordNode * next = cur->next;
        free(cur->word);
        free(cur);
        cur = next;
    }
    free(list);
}

/* Copies the word into a new WordNode on the list
*/
WordNode * add_word(WordList * list, char * word, size_t word_len) {
    WordNode * retval = malloc(sizeof(WordNode));
    if ( NULL == retval ) {
        perror("Error allocating WordNode");
        return NULL;
    }

    char * new_word = calloc(word_len + 1, sizeof(char));
    strncpy(new_word, word, word_len + 1);
    new_word[word_len] = '\0';

    retval->next = NULL;
    retval->word = new_word;

    if ( NULL == list->head ) {
        list->head = retval;
        list->tail = retval;
    } else {
        list->tail->next = retval;
        list->tail = retval;
    }

    return retval;
}

void destroy_lists(WordListByFirst * lists) {
    for ( int i = 0; i < COUNTOF(lists->letters); i++ ) {
        destroy_list(lists->letters[i]);
    }
    free(lists);
}
WordListByFirst * create_lists() {
    WordListByFirst * retval = malloc(sizeof(WordListByFirst));
    if ( NULL == retval ) {
        perror("Error allocating lists");
        return NULL;
    }
    // Initialize all of these in case we fail during allocation later,
    // then they will still free properly when destroyed.
    for ( int i = 0; i < COUNTOF(retval->letters); i++ ) {
        retval->letters[i] = NULL;
    }
    for ( int i = 0; i < COUNTOF(retval->letters); i++ ) {
        retval->letters[i] = create_list();
        if ( NULL == retval->letters[i] ) {
            destroy_lists(retval);
            return NULL;
        }
    }
    return retval;
}

WordNode * add_word_to_lists(WordListByFirst * lists, char * word,
        size_t word_len) {
    if ( word_len < 1 || NULL == word ) {
        fprintf(stderr, "Invalid word length or word is NULL: %li", word_len);
        return NULL;
    }

    WordList * list = lists->letters[LETT_TO_IND(word[0])];
    return add_word(list, word, word_len);
}


typedef struct {
    WordNode * entry;
} WordListIter;
WordListIter iterate_list(WordList * list) {
    WordListIter retval = { list->head };
    return retval;
}
bool iterate_list_not_done(WordListIter * iter) {
    return NULL != iter->entry;
}
void iterate_list_next(WordListIter * iter) {
    if ( NULL != iter->entry ) {
        iter->entry = iter->entry->next;
    }
}


// Input an array of char * words, last entry must be NULL
size_t unique_chars(char ** words) {
    size_t letter_count[26] = { 0 };
    size_t unique_char_cnt = 0;
    char ** cur_word = words;
    while ( NULL != * cur_word ) {
        char * cur = * cur_word;
        while ( '\0' != *cur ) {
            if ( 0 == letter_count[LETT_TO_IND(*cur)] ) {
                unique_char_cnt += 1;
            }
            letter_count[LETT_TO_IND(*cur)] += 1;
            cur++;
        }
        cur_word++;
    }
    return unique_char_cnt;
}


int main(int argc, char * argv[]) {
    // Read in the characters
    char line[LETTER_IN_BUFFER_LEN] = "";
    if ( NULL == fgets(line, sizeof(line), stdin) ) {
        fprintf(stderr, "Error reading line.\n");
        return 1;
    }

    // Build the data structures
    // An array of 26 pointer/null vals, pointer to a struct with the char and
    // each of the other chars it can go to next
    LetterBoxed * box = create_box(line);
    if ( NULL == box ) {
        return 1;
    }

    // Build a linked-list of all possible words
    // Go down the dictionary, check each word first letter, check if it
    // can go to next, etc.
    FILE * dict_file = fopen(DEFAULT_DICT, "r");
    if ( NULL == dict_file ) {
        perror("Error opening dictionary");
        destroy_box(box);
        return 1;
    }

    WordListByFirst * possible_words = create_lists();

    char * line_ptr = NULL;
    size_t line_len = 0;

    while ( true ) {
        ssize_t line_read = getline(&line_ptr, &line_len, dict_file);
        if ( -1 == line_read ) {
            break;
        }

        size_t word_len = line_read;
        if ( '\0' == line_ptr[line_read] ) {
            word_len -= 1;
        }

        // Convert the word to lowercase in-place, and detect non-ascii chars
        if ( 0 == tolower_ascii_str(line_ptr, word_len) ) {
            continue;
        }

        if ( is_word_in_box(box, line_ptr, word_len) ) {
            add_word_to_lists(possible_words, line_ptr, word_len);
        }
    }

    free(line_ptr);
    fclose(dict_file);

    // Let's see all the possible words to build
    /*
    for ( int i = 0; i < COUNTOF(possible_words->letters); i++ ) {
        printf("Possible words %c:\n", i+'a');
        for (
            WordListIter poss_iter = iterate_list(possible_words->letters[i]);
            iterate_list_not_done(&poss_iter);
            iterate_list_next(&poss_iter) )
        {
            printf("%s\n", poss_iter.entry->word);
        }
    }
    */

    /*
    Algorithm to actually solve this:
    Goal - minimize the number of words required to use every character of the input line.
    Do a depth-first search, where the possible words form the nodes, and the next step in each search step is the words that start with the last letter of the current node.  Search down until either you use all the letters in the input line, or until you've used more words than the previous best.
    */

    /*
    That's annoying to implement in C.  Instead, I'm going to output the possible words with the largest numbers of unique characters.
    */
    size_t unique_counts[13] = { 0 };
    size_t max_unique = 0;
    for ( int i = 0; i < COUNTOF(possible_words->letters); i++ ) {
        for (
            WordListIter poss_iter = iterate_list(possible_words->letters[i]);
            iterate_list_not_done(&poss_iter);
            iterate_list_next(&poss_iter)
        ) {
            char * word_list[] = {poss_iter.entry->word, NULL};
            size_t un = unique_chars(word_list);
            if ( un >= COUNTOF(unique_counts) ) {
                fprintf(stderr, "Invalid unique counts: %li\n", un);
            }
            unique_counts[un] += 1;

            if ( un > max_unique ) {
                max_unique = un;
            }
        }
    }
    printf("Unique count distribution:\n");
    for ( int i = 0; i < COUNTOF(unique_counts); i++ ) {
        printf("%li ", unique_counts[i]);
    }
    printf("\n");

    // Now - print off all words with the max_unique chars, or two fewer
    for ( size_t target_unique = max_unique; target_unique >= max_unique - 2;
            target_unique--
    ) {
        printf("Highly unique words: %li\n", target_unique);

        for ( int i = 0; i < COUNTOF(possible_words->letters); i++ ) {
            for (
                WordListIter poss_iter =
                    iterate_list(possible_words->letters[i]);
                iterate_list_not_done(&poss_iter);
                iterate_list_next(&poss_iter)
            ) {
                char * word_list[] = {poss_iter.entry->word, NULL};
                size_t un = unique_chars(word_list);
                if ( un == target_unique ) {
                    printf("%s\n", poss_iter.entry->word);
                }
            }
        }
    }

    // Let's try all solutions with just two words...
    char shortest_two[100] = INIT_STR;
    char shortest_three[100] = INIT_STR;
    char temp_buff[COUNTOF(shortest_two)] = { 0 };
    for ( int i = 0; i < COUNTOF(possible_words->letters); i++ ) {
        for (
            WordListIter poss_iter = iterate_list(possible_words->letters[i]);
            iterate_list_not_done(&poss_iter);
            iterate_list_next(&poss_iter)
        ) {
            char * first_word = poss_iter.entry->word;
            char first_word_last_letter = first_word[strlen(first_word) - 1];

            for ( int j = 0; j < COUNTOF(possible_words->letters); j++ ) {
                for (
                    WordListIter poss_iter_2 =
                        iterate_list(possible_words->letters[j]);
                    iterate_list_not_done(&poss_iter_2);
                    iterate_list_next(&poss_iter_2)
                ) {
                    char * second_word = poss_iter_2.entry->word;
                    char second_word_last_letter =
                        second_word[strlen(second_word) - 1];
                    if ( first_word_last_letter != second_word[0] ) {
                        continue;
                    }

                    char * word_list[] = {first_word, second_word, NULL};
                    size_t un = unique_chars(word_list);

                    if ( un == 12 ) {
                        printf("Two word solution: %s %s\n", first_word,
                            second_word);

                        snprintf(temp_buff, sizeof(temp_buff), "%s %s",
                            first_word, second_word);
                        if ( strlen(temp_buff) < strlen(shortest_two) ) {
                            strncpy(shortest_two, temp_buff,
                                sizeof(shortest_two));
                        }
                    }
                    for ( int k = 0; k < COUNTOF(possible_words->letters);
                            k++
                    ) {
                        for (
                            WordListIter poss_iter_3 =
                                iterate_list(possible_words->letters[k]);
                            iterate_list_not_done(&poss_iter_3);
                            iterate_list_next(&poss_iter_3)
                        ) {
                            char * third_word = poss_iter_3.entry->word;
                            if ( second_word_last_letter != third_word[0] ) {
                                continue;
                            }

                            char * word_list[] = {first_word, second_word,
                                third_word, NULL};
                            size_t un = unique_chars(word_list);

                            if ( un == 12 ) {
                                printf("Three word solution: %s %s %s\n",
                                    first_word, second_word, third_word);

                                snprintf(temp_buff, sizeof(temp_buff),
                                    "%s %s %s", first_word, second_word,
                                    third_word);
                                if ( strlen(temp_buff) < strlen(shortest_three)
                                ) {
                                    strncpy(shortest_three, temp_buff,
                                        sizeof(shortest_three));
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    printf("Shortest\nTwo word solution: %s\n", shortest_two);
    printf("Shortest\nThree word solution: %s\n", shortest_three);

    destroy_lists(possible_words);
    destroy_box(box);
    return 0;
}
