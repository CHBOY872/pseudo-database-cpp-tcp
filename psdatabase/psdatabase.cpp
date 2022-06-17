#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include "psdatabase.hpp"

void DbHandler::PersonCopy(const struct DbObject *from, struct DbObject *to)
// the function for copy person structure in other person structure
{
    to->id = from->id;
    strcpy(to->name, from->name);
    strcpy(to->surname, from->surname);
    to->pesel = from->pesel;
}

int DbHandler::SearchBin(_pesel_len pesel, const struct DbObjectPosition *arr,
                         int len, int low, int high)
{                               // search the person by PESEL
                                // in dynamic array,
    int mid = (low + high) / 2; // return the position in
                                // the dynamic array, else -1
    if (low < 0 || high < 0)
        return -1;
    if (low > high)
        return -1;

    if (pesel > arr[mid].pesel)
        return SearchBin(pesel, arr, len, mid + 1, high);
    else if (pesel < arr[mid].pesel)
        return SearchBin(pesel, arr, len, low, mid - 1);
    else if (arr[mid].pesel == pesel)
        return mid;
    return -1;
}

void DbHandler::Sort(struct DbObjectPosition *array, int len)
// Sort the array of
// struct person_position
// saving indexes of
// begining condition
{
    int check = 1;
    struct DbObjectPosition buf;
    while (check)
    {
        check = 0;
        for (int i = 0; i < len - 1; i++)
        {
            if (array[i].pesel > array[i + 1].pesel)
            {
                PersonCopy(&array[i + 1], &buf);
                buf.pos = array[i + 1].pos;
                PersonCopy(&array[i], &array[i + 1]);
                array[i + 1].pos = array[i].pos;
                PersonCopy(&buf, &array[i]);
                array[i].pos = buf.pos;
                check++;
            }
        }
    }
}

long DbHandler::GetFileLen(const char *path) // the function for getting
                                             // the size of file
{
    FILE *from = fopen(path, "r");
    if (from == NULL)
    {
        return -1;
    }
    fseek(from, 0, SEEK_END);
    int len = ftell(from);
    fclose(from);
    return len;
}

void DbHandler::GetCurrentId()              // get current id from ID_MAX.bin
{                                           // if there is not any file
                                            // with ID_MAX.bin name
    FILE *from = fopen(ID_FILE_NAME, "rb"); // if reading was unsuccessful,
                                            // create PERSON.dat and return 0
    struct DbObject per;                    // else return the last written id
    if (!from)
    {
        mkdir("ID", 0777);
        creat(ID_FILE_NAME, 0777);
        from = fopen(TEXT_DATABASE_NAME, "r");
        if (!from)
        {
            mkdir("PERSON_DAT", 0777);
            creat(TEXT_DATABASE_NAME, 0777);
            id = 0;
            return;
        }
        if (fseek(from, -(writing_format_len + 1), SEEK_END) == -1)
        {
            fclose(from);
            id = 0;
            return;
        }
        fscanf(from, WRITING_FORMAT, &per.id, per.name,
               per.surname, &per.pesel);
        fclose(from);
        id = ++per.id;
        return;
    }
    fread(&id, sizeof(_id_len), 1, from);
    fclose(from);
}

void DbHandler::Add(struct DbObject *per) // write the person
                                          // in the last position of
                                          // the DB, make the
                                          // increment of ID
{
    FILE *to = fopen(TEXT_DATABASE_NAME, "a");
    if (!to)
    {
        creat(TEXT_DATABASE_NAME, 0777);
        to = fopen(TEXT_DATABASE_NAME, "a");
    }
    fprintf(to, WRITING_FORMAT, id, per->name, per->surname, per->pesel);
    id++;
    fclose(to);
}

void DbHandler::EditByPesel(_pesel_len pesel, const struct DbObject *per)
// edit the person by his PESEL
{ // if there is not any person with that PESEL, return -1
    struct DbObject temp;
    _position stat = GetByPesel(pesel, &temp);
    if (-1 == stat)
        return;
    FILE *where = fopen(TEXT_DATABASE_NAME, "r+");
    if (!where)
        perror("error #TEXT_DATABASE_NAME_OPENING_EDIT_BY_PESEL");

    fseek(where, writing_format_len * stat, SEEK_SET);
    fprintf(where, WRITING_FORMAT, temp.id, per->name,
            per->surname, per->pesel);
    fclose(where);
}

void DbHandler::SaveId() // save id before exiting from the program
{
    FILE *to = fopen(ID_FILE_NAME, "wb");
    fwrite(&id, sizeof(_id_len), 1, to);
    fclose(to);
}

void DbHandler::ShiftDatabase() // make the new copy of DB
                                // skipping deleted people
{
    struct DbObject per;
    FILE *from = fopen(TEXT_DATABASE_NAME, "r");
    if (!from)
        perror("error #TEXT_DATABASE_NAME_OPENING_SHIFT_DATABASE");
    FILE *to = fopen(TEXT_DATABASE_NAME_NEW, "w+");
    if (!to)
        perror("error #TEXT_DATABASE_NAME_NEW_OPENING_SHIFT_DATABASE");
    while (fscanf(from, WRITING_FORMAT, &per.id,
                  per.name, per.surname, &per.pesel) != EOF)
    {
        if (per.id == -1)
            continue;
        fprintf(to, WRITING_FORMAT, per.id, per.name, per.surname, per.pesel);
    }
    fclose(from);
    fclose(to);
    if (remove(TEXT_DATABASE_NAME) == -1)
        perror("error #TEXT_DATABASE_REMOVE_SHIFT_DATABASE");
    if (rename(TEXT_DATABASE_NAME_NEW, TEXT_DATABASE_NAME) == -1)
        perror("error #TEXT_DATABASE_RENAME_SHIFT_DATABASE");
}

_position DbHandler::GetByPesel(_pesel_len pesel, struct DbObject *per)
{ // search the person in the DB,
    // return position in the file, else if not found, -1
    long file_len = GetFileLen(TEXT_DATABASE_NAME),
         count_of_people = file_len / (writing_format_len),
         position = 0;
    long iterations =
        (count_of_people <= max_person_in_dynamic_array)
            ? 1
            : (count_of_people / max_person_in_dynamic_array) + 1;
    long person_in_file_left = count_of_people;
    long i;
    int j, count_of_people_in_array, position_in_array = 0;
    struct DbObjectPosition *people_arr;
    FILE *from = fopen(TEXT_DATABASE_NAME, "r");
    if (!from)
        perror("error #TEXT_DATABASE_NAME_OPENING_GET_BY_PESEL");

    for (i = 0; i < iterations; i++)
    {
        if (person_in_file_left < max_person_in_dynamic_array)
            count_of_people_in_array = person_in_file_left;
        else
        {
            count_of_people_in_array = max_person_in_dynamic_array;
            person_in_file_left -= max_person_in_dynamic_array;
        }
        people_arr = new DbObjectPosition
            [sizeof(struct DbObjectPosition) * count_of_people_in_array];
        for (j = 0;
             (j < count_of_people_in_array) &&
             (fscanf(from, WRITING_FORMAT, &people_arr[j].id,
                     people_arr[j].name, people_arr[j].surname,
                     &people_arr[j].pesel) != EOF);
             j++)
            people_arr[j].pos = j;
        Sort(people_arr, count_of_people_in_array);
        position_in_array = SearchBin(pesel, people_arr,
                                      count_of_people_in_array, 0,
                                      count_of_people_in_array - 1);
        if (position_in_array != -1)
        {
            position += people_arr[position_in_array].pos;
            PersonCopy(&people_arr[position_in_array], per);
            delete[] people_arr;
            fclose(from);
            return position;
        }
        delete[] people_arr;
        position += count_of_people_in_array;
    }
    fclose(from);
    return -1;
}

void DbHandler::RemoveByPesel(_pesel_len pesel) // delete the
                                                // person by his PESEL,
{                                               // if there is not
                                                // any person with
                                                // that PESEL, return -1
    struct DbObject per;
    int stat = GetByPesel(pesel, &per);
    if (stat == -1)
        return;
    FILE *where = fopen(TEXT_DATABASE_NAME, "r+");
    if (!where)
        perror("error #TEXT_DATABASE_NAME_OPENING_DELETE_BY_PESEL");

    fseek(where, writing_format_len * stat, SEEK_SET);
    fprintf(where, WRITING_FORMAT, (_id_len)-1, "NULL", "NULL", (_pesel_len)0);
    fclose(where);
}

DbHandler::DbHandler(_id_len _id) : id(_id) {}

DbHandler *DbHandler::Start()
{
    DbHandler *db_handler = new DbHandler();
    db_handler->GetCurrentId();
    return db_handler;
}

DbHandler::~DbHandler()
{
    Quit();
}