#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LD_LIBRARY_PATH "LD_LIBRARY_PATH"
#define LD_PRELOAD "LD_PRELOAD"
#define PRELOAD_FILE_NAME "lindbergh.so"

// List of all lindbergh executables known, not including the test executables
char *games[] = {"amiM.elf", "abc", "hod4M.elf", "lgj_final", "vt3", "id4.elf", "id5.elf", "lgjsp_app", "gsevo", "vf5", "apacheM.elf", "END"};

/**
 * Tests if the game uses a seperate elf for test mode
 * and updates the command line to this elf.
 *
 * @param name The command path to run
 */
void testModePath(char *name)
{
    // Check if a different testmode elf is used
    if (strcmp(name, "./hod4M.elf") == 0)
    {
        strcpy(name, "./hod4testM.elf");
        return;
    }

    if (strcmp(name, "./Jennifer") == 0)
    {
        strcpy(name, "../JenTest/JenTest");
        return;
    }

    if (strcmp(name, "./apacheM.elf") == 0)
    {
        strcpy(name, "./apacheTestM.elf");
        return;
    }

    // Otherwise add the standard -t to the end
    strcat(name, " -t");
}

/**
 * Makes sure the environment variables are set correctly
 * to run the game.
 */
void setEnvironmentVariables()
{
    // Ensure the library path is set correctly
    char libraryPath[128] = {0};

    const char *currentLibraryPath = getenv(LD_LIBRARY_PATH);
    if (currentLibraryPath != NULL)
    {
        strcat(libraryPath, currentLibraryPath);
        strcat(libraryPath, ":");
    }

    strcat(libraryPath, ".:lib");

    setenv(LD_LIBRARY_PATH, libraryPath, 1);

    // Ensure the preload path is set correctly
    setenv(LD_PRELOAD, PRELOAD_FILE_NAME, 1);
}

/**
 * Prints the usage for the loader
 */
void printUsage(char *argv[])
{
    printf("%s [GAME_PATH] [OPTIONS]\n", argv[0]);
    printf("Options:\n");
    printf("  --test | -t    Runs the test mode\n");
    printf("  --help         Displays this usage text\n");
}

/**
 * Small utility to automatically detect the game and run it without
 * having to type a long string in.
 */
int main(int argc, char *argv[])
{

    // Ensure environment variables are set correctly
    setEnvironmentVariables();

    // Look for the games
    struct dirent *ent;
    DIR *dir = opendir(".");

    if (dir == NULL)
    {
        printf("Error: Could not list files in current directory.\n");
        return EXIT_FAILURE;
    }

    char *game = NULL;

    int lindberghSharedObjectFound = 0;

    while ((ent = readdir(dir)) != NULL)
    {

        if (ent->d_type != DT_REG)
            continue;

        if (strcmp(ent->d_name, PRELOAD_FILE_NAME) == 0)
        {
            lindberghSharedObjectFound = 1;
            continue;
        }

        int index = 0;
        while (1)
        {
            if (strcmp(games[index], "END") == 0)
                break;

            if (strcmp(ent->d_name, games[index]) == 0)
            {
                game = games[index];
                break;
            }

            index++;
        }

        if (game != NULL)
            break;
    }

    closedir(dir);

    if (argc > 1)
    {
        if (strcmp(argv[1], "--help") == 0)
        {
            printUsage(argv);
            return EXIT_SUCCESS;
        }
    }

    if (!lindberghSharedObjectFound)
    {
        printf("Error: The preload object lindbergh.so was not found in this directory.\n");
        return EXIT_FAILURE;
    }

    if (game == NULL)
    {
        printf("Error: No lindbergh game found in this directory.\n");
        return EXIT_FAILURE;
    }

    // Build up the command to start the game
    char command[128];
    strcpy(command, "./");
    strcat(command, game);

    if (argc > 1)
    {
        if (strcmp(argv[1], "-t") == 0 || strcmp(argv[1], "--test") == 0)
        {
            testModePath(command);
        }
    }

    printf("$ %s\n", command);

    return system(command);
}
