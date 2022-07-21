#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <json-c/json.h>

// Buffer structure, holds a char pointer and an int to keep track of size because apparently this is how you do things in C
struct bufferStruct {
    char * charBuffer;
    size_t size;
};

// Counts characters, prints the english language letter counts of the char pointer passed to it, elementary programming, literally one of the first exercises in K&R
void countCharacters(char * textToCount) {
    // Variable initializaton
    int characterOccurrences[26] = {0};
    int c;

    // Parsing loop
    for (int i = 0; i < strlen(textToCount); i++) {
        c = (int) *(textToCount + i);
        if ((c > 64) && (c < 91)) {
            characterOccurrences[c - 65]++;
        } else if ((c > 96) && (c < 123)) {
            characterOccurrences[c - 97]++;
        }
    }

    // Printing loop
    for (int i = 0; i < 26; i++) {
        printf("%c\t%d\n", i + 65, characterOccurrences[i]);
    }
}

size_t dataToBuffer(void * incoming, size_t size, size_t nitems, void * foreignBuffer) {
    // Creates relevant variables for the dataToBuffer callback function
    int incomingSize;
    struct bufferStruct * localBuffer;
    char * copyTo;

    incomingSize = size * nitems; // Assigns incomingSize, which must be returned from the dataToBuffer with a value of size * nitems for the program to operate correctly
    localBuffer = (struct bufferStruct *) foreignBuffer; // Casts the foreignBuffer void pointer passed to the function into a friendlier bufferStruct pointer

    copyTo = realloc(localBuffer->charBuffer, localBuffer->size + incomingSize + 1); // Assigns the copyTo char pointer to a reallocation of the original buffer pointer

    localBuffer->charBuffer = copyTo; // Changes the address of the charBuffer pointer within the localBuffer object to the new memory location, as well as the original foreignBuffer by proxy
    memcpy(&(localBuffer->charBuffer[localBuffer->size]), incoming, incomingSize); // Appends the contents of the incoming data, newly downloaded from the web, to the existing character buffer
    localBuffer->size += incomingSize; // Expands the size attribute of the localBuffer object to respect the increased size of the charBuffer allocation
    localBuffer->charBuffer[localBuffer->size] = 0; // Zeroes out everything following the important part of the charBuffer allocation, just incase

    return incomingSize;
}

char * parseJSON(struct bufferStruct sourceJSON) {
    // Creates JSON related variables. JSON-C is actually a pretty funny library.
    struct json_object * root;
    struct json_object * dataR;
    struct json_object * childrenR;
    struct json_object * post0;
    struct json_object * data0;
    struct json_object * selftext;

    root = json_tokener_parse(sourceJSON.charBuffer);
    
    // Crawls a JSON tree, pretty self-explanatory actually
    dataR = json_object_object_get(root, "data");
    childrenR = json_object_object_get(dataR, "children");
    post0 = json_object_array_get_idx(childrenR, 0);
    data0 = json_object_object_get(post0, "data");
    selftext = json_object_object_get(data0, "selftext");
    
    // Copies the selftext into the return value safely
    int textSize = strlen(json_object_get_string(selftext));
    char * retVal = (char *) malloc(textSize);
    strcpy(retVal, json_object_get_string(selftext));

    return retVal;
}

int main() {
    // Declares all web-request related variables, including the libcurl handle, the buffer struct, and the headers linked list (in that order)
    CURL * myCurlRequest;
    CURLcode result;
    struct bufferStruct myBuffer;
    struct curl_slist * headers = NULL;
    headers = curl_slist_append(headers, "Authorization: bearer <ACCESS_TOKEN>"); // Obtain a Reddit API access token using the steps available in the Readme or in the Reddit API documentation
    headers = curl_slist_append(headers, "User-Agent: <PLATFORM>:<CLIENT_ID>:v<VERSION> (by <REDDIT USERNAME>)"); // Set up your own bot using readme/api docs, copy the parameters into this

    // Prepares the myBuffer struct, which will store what we download from the remote API
    myBuffer.charBuffer = malloc(1 * sizeof(char));
    myBuffer.size = 0;
    
    // Initializes the curl library, using all additional modules
    curl_global_init(CURL_GLOBAL_ALL);
    myCurlRequest = curl_easy_init();

    // Sets all options for the curl request
    curl_easy_setopt(myCurlRequest, CURLOPT_URL, "<TARGET SUBREDDIT>"); // Please enter the subreddit; not the post, you want to target. Individual posts can't be targeted yet, sorry.
    curl_easy_setopt(myCurlRequest, CURLOPT_WRITEFUNCTION, dataToBuffer);
    curl_easy_setopt(myCurlRequest, CURLOPT_WRITEDATA, (void *) &myBuffer);
    curl_easy_setopt(myCurlRequest, CURLOPT_HTTPHEADER, headers);

    result = curl_easy_perform(myCurlRequest);
    if (result != CURLE_OK) {
        printf("Error with the curl performance, exiting!\n");
        exit(1);
    }

    countCharacters(parseJSON(myBuffer)); // The magic line that parses the buffer into json, then counts the characters. Someday this will use a return value, but it's 12:20 and I'm tired.

    // Cleanup code
    free(myBuffer.charBuffer);
    curl_easy_cleanup(myCurlRequest);
    curl_global_cleanup();
    }z