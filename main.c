#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILENAME "url.txt"
#define OUTPUT_FILE ".clang-format"

// A struct to hold the response data from the URL
struct MemoryStruct {
  char*  memory;
  size_t size;
};

// A write callback function to store the fetched data in the memory struct
static size_t WriteMemoryCallback(void* contents, size_t size, size_t nmemb, void* userp) {
  size_t               realsize = size * nmemb;
  struct MemoryStruct* mem = (struct MemoryStruct*)userp;

  char* ptr = realloc(mem->memory, mem->size + realsize + 1);
  if (ptr == NULL) {
    printf("Not enough memory (realloc returned NULL)\n");
    return 0;
  }

  mem->memory = ptr;
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0; // Null-terminate the string

  return realsize;
}

int main(int argc, char* argv[]) {
  // Check if any arguments are passed
  if (argc < 2) {
    printf("Usage: %s <command> [options]\n", argv[0]);
    return 1;
  }

  // Handle the 'init' command
  if (strcmp(argv[1], "init") == 0) {
    char url[256];

    // Prompt the user for a URL
    printf("Please enter the URL: ");
    fgets(url, sizeof(url), stdin);

    // Remove the newline character at the end of the URL, if present
    size_t len = strlen(url);
    if (len > 0 && url[len - 1] == '\n') {
      url[len - 1] = '\0';
    }

    // Open the file to save the URL
    FILE* file = fopen(FILENAME, "w");
    if (file == NULL) {
      perror("Error opening file");
      return 1;
    }

    // Write the URL to the file
    fprintf(file, "%s\n", url);
    fclose(file);

    printf("URL saved to %s\n", FILENAME);

  } else if (strcmp(argv[1], "setup") == 0) {
    // Check if url.txt exists
    FILE* file = fopen(FILENAME, "r");
    if (file == NULL) {
      printf("Error: %s not found. Run 'init' command first.\n", FILENAME);
      return 1;
    }

    // Read the URL from url.txt
    char url[256];
    if (fgets(url, sizeof(url), file) == NULL) {
      printf("Error: Could not read URL from %s.\n", FILENAME);
      fclose(file);
      return 1;
    }
    fclose(file);

    // Remove the newline character from the URL if it exists
    size_t len = strlen(url);
    if (len > 0 && url[len - 1] == '\n') {
      url[len - 1] = '\0';
    }

    // Initialize libcurl
    CURL*               curl;
    CURLcode            res;
    struct MemoryStruct chunk;

    chunk.memory = malloc(1); // Start with a small memory buffer
    chunk.size = 0;           // Initially, no data

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
      // Set the URL to fetch
      curl_easy_setopt(curl, CURLOPT_URL, url);

      // Set the callback function to store the data
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

      // Pass the memory struct to the callback function
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);

      // Perform the HTTP request
      res = curl_easy_perform(curl);

      // Check for errors
      if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        free(chunk.memory); // Free the memory if there's an error
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return 1;
      }

      // Write the fetched data to .clang-format file
      FILE* output_file = fopen(OUTPUT_FILE, "w");
      if (output_file == NULL) {
        perror("Error creating .clang-format");
        free(chunk.memory);
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return 1;
      }

      fwrite(chunk.memory, 1, chunk.size, output_file);
      fclose(output_file);

      printf("File saved to %s\n", OUTPUT_FILE);

      // Cleanup
      free(chunk.memory);
      curl_easy_cleanup(curl);
    }

    curl_global_cleanup();

  } else if (strcmp(argv[1], "hello") == 0) {
    printf("Hello, world!\n");
  } else if (strcmp(argv[1], "sum") == 0) {
    if (argc != 4) {
      printf("Usage: %s sum <num1> <num2>\n", argv[0]);
      return 1;
    }

    int num1 = atoi(argv[2]);
    int num2 = atoi(argv[3]);
    int result = num1 + num2;

    printf("The sum of %d and %d is: %d\n", num1, num2, result);
  } else if (strcmp(argv[1], "help") == 0) {
    printf("Available commands:\n");
    printf("  init           Prompts for a URL and saves it to a file\n");
    printf("  setup          Fetches data from the saved URL and saves it as "
           ".clang-format\n");
    printf("  hello          Print 'Hello, world!'\n");
    printf("  sum <num1> <num2>   Calculate the sum of two numbers\n");
    printf("  help           Display this help message\n");
  } else {
    printf("Unknown command: %s\n", argv[1]);
    printf("Try '%s help' for more information.\n", argv[0]);
  }

  return 0;
}
