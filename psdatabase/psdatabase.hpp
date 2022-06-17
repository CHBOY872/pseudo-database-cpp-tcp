#ifndef PSDATABASE_SENTER
#define PSDATABASE_SENTER

#include <string.h>

#define ID_FILE_NAME "ID/ID_MAX.bin"                       // the path to file
                                                           // where the last
                                                           // id saves
#define TEXT_DATABASE_NAME "PERSON_DAT/PERSON.dat"         // the path to
                                                           // database
#define TEXT_DATABASE_NAME_NEW "PERSON_DAT/PERSON_NEW.dat" // the path to new
                                                           // database for
                                                           // shifting database

#define PRINT_FORMAT "%40lld %50s %50s %40lld\n"     // format for
                                                     // showing all
                                                     // database
#define WRITING_FORMAT "%40lld %250s %250s %40lld\n" // format for writing to
                                                     // database, reading from
                                                     // database

enum
{
    writing_format_len = 584,
    buffer_size = 250,
    max_person_in_dynamic_array = 50
};

typedef long long _id_len;             // variable for id
typedef unsigned long long _pesel_len; // variable for PESEL
typedef char _byte;                    // variable for byte
typedef long _position;                // variable for position in file

class DbHandler
{
    _id_len id;

    DbHandler(_id_len _id = 0);

public:
    ~DbHandler();
    static DbHandler *Start();
    void Quit()
    {
        SaveId();
        ShiftDatabase();
    }

    _id_len GetId() { return id; }
    void Add(struct DbObject *per);
    void EditByPesel(_pesel_len pesel, const struct DbObject *per);
    void RemoveByPesel(_pesel_len pesel);
    _position GetByPesel(_pesel_len pesel, struct DbObject *per);

private:
    void GetCurrentId();
    void SaveId();

    void ShiftDatabase();

    static void PersonCopy(const struct DbObject *from, struct DbObject *to);
    static int SearchBin(_pesel_len pesel, const struct DbObjectPosition *arr,
                         int len, int low, int high);
    static void Sort(struct DbObjectPosition *array, int len);
    static long GetFileLen(const char *path);
};

struct DbObject
{
    _id_len id;
    char name[buffer_size];
    char surname[buffer_size];
    _pesel_len pesel;

    DbObject() : id(-1), pesel(0)
    {
        bzero(name, buffer_size);
        bzero(surname, buffer_size);
    }
    DbObject(_id_len _id, const char *_name,
             const char *_surname, _pesel_len _pesel) : id(_id), pesel(_pesel)
    {
        strcpy(name, _name);
        strcpy(surname, _surname);
    }
};

struct DbObjectPosition : public DbObject
{
    _byte pos;
    DbObjectPosition(_id_len _id, const char *_name,
                     const char *_surname, _pesel_len _pesel)
        : DbObject(_id, _name, _surname, _pesel) {}
    DbObjectPosition() {}
};

#endif