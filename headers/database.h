int UpdateDatabase(void);
int LoadDatabase(void);
int SetExit(user *currentuser,int whichroom,int direction,int exit);
int CreateRoom(user *currentuser,char *roomdirection);
int SetObjectAttributes(user *currentuser,char *object,char *attributes);
int SetOwner(user *currentuser,char *objectname,char *n);
int CopyObject(user *currentuser,char *ObjectName,int DestinationRoom);
int GenerateObjects(void);
int PickUpObject(user *currentuser,char *o);
int DropObject(user *currentuser,char *o);
int CreateObject(user *currentuser,char *objname);
int DeleteObject(user *currentuser,char *o);
int RenameObject(user *currentuser,char *o,char *n);
int look(user *currentuser,char *n);
int CopyFile(char *source,char *destination);
int describe(user *currentuser,char *o,char *d);
void SetDatabaseUpdateFlag(void);
void ClearDatabaseUpdateFlag(void);
char *GetDirectionName(int direction);
int GetRoomFlags(int RoomNumber);
char *GetRoomName(int RoomNumber);
room *GetRoomPointer(int RoomNumber);
int GetRoomMonsterEvil(int RoomNumber,int RoomMonster);
char *GetRoomMonsterName(int RoomNumber,int RoomMonster);
int GetNumberOfMonstersInRoom(int RoomNumber);
int IsObjectInRoom(char *name,int RoomNumber);


