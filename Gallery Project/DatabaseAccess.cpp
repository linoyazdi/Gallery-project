#include <map>
#include <algorithm>

#include "ItemNotFoundException.h"
#include "DatabaseAccess.h"



void DatabaseAccess::printAlbums()
{
	if (m_albums.empty()) {
		throw MyException("There are no existing albums.");
	}
	std::cout << "Album list:" << std::endl;
	std::cout << "-----------" << std::endl;
	for (const Album& album : m_albums) {
		std::cout << std::setw(5) << "* " << album;
	}
}

bool DatabaseAccess::open()
{
	int doesFileExist = _access(dbFileName.c_str(), 0);
	int res = sqlite3_open(dbFileName.c_str(), &db);

	if (res != SQLITE_OK)
	{
		db = nullptr;
		std::cout << "Failed to open DB" << std::endl;
		return false;
	}

	if (doesFileExist == -1) {
		// init database
		char* sqlStatementUsers = "CREATE TABLE IF NOT EXISTS USERS (ID INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, NAME TEXT NOT NULL);";
		char* sqlStatementAlbums = "CREATE TABLE IF NOT EXISTS ALBUMS (ID INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, NAME TEXT NOT NULL, CREATION_DATE TEXT NOT NULL, USER_ID INTEGER FOREIGN KEY REFERENCES USERS(ID) NOT NULL);";
		char* sqlStatementPictures = "CREATE TABLE IF NOT EXISTS PICTURES (ID INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, NAME TEXT NOT NULL, LOCATION TEXT NOT NULL, CREATION_DATE TEXT NOT NULL, ALBUM_ID INTEGER FOREIGN KEY REFERENCES ALBUMS(ID) NOT NULL);";
		char* sqlStatementTags = "CREATE TABLE IF NOT EXISTS TAGS (ID INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, PICTURE_ID INTEGER FOREIGN KEY REFERENCES PICTURES(ID) NOT NULL, USER_ID INTEGER FOREIGN KEY REFERENCES USERS(ID) NOT NULL);";

		if (!(runSqlCommand(sqlStatementUsers) && runSqlCommand(sqlStatementAlbums) && runSqlCommand(sqlStatementPictures) && runSqlCommand(sqlStatementTags)))
		{
			std::cout << "Failed to create DB" << std::endl;
			dropTables();
			sqlite3_close(db);
			db = nullptr;
			remove(dbFileName.c_str());
			return false;
		}
	}

	return true;
}

void DatabaseAccess::close()
{
	sqlite3_close(db);
	db = nullptr;
}


bool DatabaseAccess::runSqlCommand(std::string sqlStatement)
{
	char* command = new char[sqlStatement.length() + 1];
	strcpy(command, sqlStatement.c_str());

	char** errMessage = nullptr;
	bool flag = true;
	int res = sqlite3_exec(db, command, nullptr, nullptr, errMessage);

	if (res != SQLITE_OK)
		flag = false;

	delete[] command;
	return flag;
}


void DatabaseAccess::clear()
{
	m_users.clear();
	m_albums.clear();
}


void DatabaseAccess::dropTables()
{
	sqlite3_exec(db, "DROP TABLE USERS;", nullptr, nullptr, nullptr);
	sqlite3_exec(db, "DROP TABLE ALBUMS;", nullptr, nullptr, nullptr);
	sqlite3_exec(db, "DROP TABLE PICTURES;", nullptr, nullptr, nullptr);
	sqlite3_exec(db, "DROP TABLE TAGS;", nullptr, nullptr, nullptr);
}


auto DatabaseAccess::getAlbumIfExists(const std::string& albumName)
{
	auto result = std::find_if(std::begin(m_albums), std::end(m_albums), [&](auto& album) { return album.getName() == albumName; });

	if (result == std::end(m_albums)) {
		throw ItemNotFoundException("Album not exists: ", albumName);
	}

	return result;
}


DatabaseAccess::DatabaseAccess()
{
	this->dbFileName = "MyDB.sqlite";
}


const std::list<Album> DatabaseAccess::getAlbums()
{
	return m_albums;
}


std::list<Album> DatabaseAccess::getAlbumsOfUser(const User& user)
{
	std::list<Album> albumsOfUser;
	for (const auto& album : m_albums) {
		if (album.getOwnerId() == user.getId()) {
			albumsOfUser.push_back(album);
		}
	}
	return albumsOfUser;
}


void DatabaseAccess::createAlbum(const Album& album)
{
	std::string strCommand = "INSERT INTO ALBUMS VALUES (" + std::to_string(album.getId()) + ", \"" + album.getName() + "\", \"" + album.getCreationDate() + "\", " + std::to_string(album.getOwnerId()) + ");";

	if (runSqlCommand(strCommand))
	{
		m_albums.push_back(album);
	}
	else
	{
		std::cout << "Failed to create album" << std::endl;
	}
}


void DatabaseAccess::deleteAlbum(const std::string& albumName, int userId)
{
	for (auto iter = m_albums.begin(); iter != m_albums.end(); iter++) {
		if (iter->getName() == albumName && iter->getOwnerId() == userId)
		{
			std::string strCommand = "DELETE FROM ALBUMS WHERE NAME = \"" + albumName + "\";";
			
			if (runSqlCommand(strCommand))
			{
				iter = m_albums.erase(iter);
			}
			else
			{
				std::cout << "Failed to delete album" << std::endl;
			}
			
			return;
		}
	}
}


bool DatabaseAccess::doesAlbumExists(const std::string& albumName, int userId)
{
	for (const auto& album : m_albums) {
		if ((album.getName() == albumName) && (album.getOwnerId() == userId)) {
			return true;
		}
	}

	return false;
}


Album DatabaseAccess::openAlbum(const std::string& albumName)
{
	for (auto& album : m_albums) {
		if (albumName == album.getName()) {
			return album;
		}
	}
	throw MyException("No album with name " + albumName + " exists");
}


void DatabaseAccess::addPictureToAlbumByName(const std::string& albumName, const Picture& picture)
{
	try
	{
		auto result = getAlbumIfExists(albumName);

		std::string strCommand = "INSERT INTO PICTURES VALUES (" + picture.getId() + std::string(", \"") + picture.getName() + std::string("\", \"") + picture.getPath() + std::string("\", \"") + picture.getCreationDate() + std::string("\", ") + std::to_string(result->getOwnerId()) + ");";

		if (runSqlCommand(strCommand))
		{
			(*result).addPicture(picture);
		}
		else
		{
			std::cout << "Failed to delete add picture to album by name" << std::endl;
		}
	}

	catch (std::exception& e)
	{
		throw e;
	}
}


void DatabaseAccess::removePictureFromAlbumByName(const std::string& albumName, const std::string& pictureName)
{
	try
	{
		auto result = getAlbumIfExists(albumName);

		std::string strCommand = "DELETE FROM PICTURES WHERE NAME = \"" + pictureName + "\");";

		if (!(runSqlCommand(strCommand)))
		{
			std::cout << "Failed to remove picture from album by name" << std::endl;
		}
		else
		{
			(*result).removePicture(pictureName);
		}
	}

	catch (std::exception& e)
	{
		throw e;
	}
}


void DatabaseAccess::tagUserInPicture(const std::string& albumName, const std::string& pictureName, int userId)
{
	try
	{
		auto result = getAlbumIfExists(albumName);

		Picture picture = (*result).getPicture(pictureName);
		std::string strCommand = "INSERT INTO TAGS (PICTURE_ID, USER_ID) VALUES (" + std::to_string(picture.getId()) + ", \"" + std::to_string(userId) + ");";

		if (!(runSqlCommand(strCommand)))
		{
			std::cout << "Failed to delete tag user in picture" << std::endl;
		}
		else
		{
			(*result).tagUserInPicture(userId, pictureName);
		}
	}

	catch (std::exception& e)
	{
		throw e;
	}
}


void DatabaseAccess::untagUserInPicture(const std::string& albumName, const std::string& pictureName, int userId)
{
	try
	{
		auto result = getAlbumIfExists(albumName);

		Picture picture = (*result).getPicture(pictureName);
		std::string strCommand = "DELETE FROM TAGS WHERE PICTURE_ID = " + std::to_string(picture.getId()) + "AND USER_ID = " + std::to_string(userId) + ";";

		if (!(runSqlCommand(strCommand)))
		{
			std::cout << "Failed to delete tag user in picture" << std::endl;
		}
		else
		{
			(*result).untagUserInPicture(userId, pictureName);
		}
	}

	catch (std::exception& e)
	{
		throw e;
	}
}


void DatabaseAccess::closeAlbum(Album&)
{
	// basically here we would like to delete the allocated memory we got from openAlbum
}


// ******************* User ******************* 


void DatabaseAccess::printUsers()
{
	std::cout << "Users list:" << std::endl;
	std::cout << "-----------" << std::endl;
	for (const auto& user : m_users) {
		std::cout << user << std::endl;
	}
}


User DatabaseAccess::getUser(int userId) {
	for (const auto& user : m_users) {
		if (user.getId() == userId) {
			return user;
		}
	}

	throw ItemNotFoundException("User", userId);
}


void DatabaseAccess::createUser(User& user)
{
	std::string strCommand = "INSERT INTO USERS VALUES (" + std::to_string(user.getId()) + ", \"" + user.getName() + "\");";

	if (runSqlCommand(strCommand))
	{
		m_users.push_back(user);
	}
	else
	{
		std::cout << "Failed to remove picture from album by name" << std::endl;
	}
}


void DatabaseAccess::deleteUser(const User& user)
{
	if (doesUserExists(user.getId())) {

		for (auto iter = m_users.begin(); iter != m_users.end(); ++iter) {
			if (*iter == user) {

				std::string strCommand = "DELETE FROM USERS WHERE ID = " + std::to_string(user.getId()) + ";";

				if (runSqlCommand(strCommand))
				{
					iter = m_users.erase(iter);
				}
				else
				{
					std::cout << "Failed to remove picture from album by name" << std::endl;
				}
				
				return;
			}
		}
	}
}


bool DatabaseAccess::doesUserExists(int userId)
{
	auto iter = m_users.begin();
	for (const auto& user : m_users) {
		if (user.getId() == userId) {
			return true;
		}
	}

	return false;
}


/*
This function deletes all the user's albums
input: the user
output: none
*/
void DatabaseAccess::deleteUsersAlbums(const User& user)
{
	try
	{
		std::list<Album> albumsList = getAlbumsOfUser(user);
		for (auto album = albumsList.begin(); album != albumsList.end(); ++album) {
			deleteAlbum(album->getName(), user.getId());
		}
	}

	catch (std::exception& e)
	{
		throw e;
	}
}


/*
This function removes all the user's tags
input: the user
output: none
*/
void DatabaseAccess::deleteUserTags(const User& user)
{
	try
	{
		for (const auto& album : m_albums) {
			const std::list<Picture>& pics = album.getPictures();

			for (const auto& picture : pics) {
				if (picture.isUserTagged(user)) {
					untagUserInPicture(album.getName(), picture.getName(), user.getId());
				}
			}
		}
	}

	catch (std::exception& e)
	{
		throw e;
	}
}


// user statistics


int DatabaseAccess::countAlbumsOwnedOfUser(const User& user)
{
	int albumsCount = 0;

	for (const auto& album : m_albums) {
		if (album.getOwnerId() == user.getId()) {
			++albumsCount;
		}
	}

	return albumsCount;
}


int DatabaseAccess::countAlbumsTaggedOfUser(const User& user)
{
	int albumsCount = 0;

	for (const auto& album : m_albums) {
		const std::list<Picture>& pics = album.getPictures();

		for (const auto& picture : pics) {
			if (picture.isUserTagged(user)) {
				albumsCount++;
				break;
			}
		}
	}

	return albumsCount;
}


int DatabaseAccess::countTagsOfUser(const User& user)
{
	int tagsCount = 0;

	for (const auto& album : m_albums) {
		const std::list<Picture>& pics = album.getPictures();

		for (const auto& picture : pics) {
			if (picture.isUserTagged(user)) {
				tagsCount++;
			}
		}
	}

	return tagsCount;
}


float DatabaseAccess::averageTagsPerAlbumOfUser(const User& user)
{
	int albumsTaggedCount = countAlbumsTaggedOfUser(user);

	if (0 == albumsTaggedCount)
	{
		return 0;
	}

	return static_cast<float>(countTagsOfUser(user)) / albumsTaggedCount;
}


User DatabaseAccess::getTopTaggedUser()
{
	std::map<int, int> userTagsCountMap;

	auto albumsIter = m_albums.begin();
	for (const auto& album : m_albums) {
		for (const auto& picture : album.getPictures()) {
			const std::set<int>& userTags = picture.getUserTags();
			for (const auto& user : userTags) {
				//As map creates default constructed values, 
				//users which we haven't yet encountered will start from 0
				userTagsCountMap[user]++;
			}
		}
	}

	if (userTagsCountMap.size() == 0) {
		throw MyException("There isn't any tagged user.");
	}

	int topTaggedUser = -1;
	int currentMax = -1;
	for (auto entry : userTagsCountMap) {
		if (entry.second < currentMax) {
			continue;
		}

		topTaggedUser = entry.first;
		currentMax = entry.second;
	}

	if (-1 == topTaggedUser) {
		throw MyException("Failed to find most tagged user");
	}

	if (!doesUserExists(topTaggedUser))
	{
		throw MyException("The most tagged user is no longer exists");
	}

	return getUser(topTaggedUser);
}


Picture DatabaseAccess::getTopTaggedPicture()
{
	int currentMax = -1;
	const Picture* mostTaggedPic = nullptr;
	for (const auto& album : m_albums) {
		for (const Picture& picture : album.getPictures()) {
			int tagsCount = picture.getTagsCount();
			if (tagsCount == 0) {
				continue;
			}

			if (tagsCount <= currentMax) {
				continue;
			}

			mostTaggedPic = &picture;
			currentMax = tagsCount;
		}
	}
	if (nullptr == mostTaggedPic) {
		throw MyException("There isn't any tagged picture.");
	}

	return *mostTaggedPic;
}


std::list<Picture> DatabaseAccess::getTaggedPicturesOfUser(const User& user)
{
	std::list<Picture> pictures;

	for (const auto& album : m_albums) {
		for (const auto& picture : album.getPictures()) {
			if (picture.isUserTagged(user)) {
				pictures.push_back(picture);
			}
		}
	}

	return pictures;
}


int DatabaseAccess::usersCallback(void* data, int argc, char** argv, char** azColName)
{
	User user;

	for (int i = 0; i < argc; i++) {
		if (std::string(azColName[i]) == "ID") {
			user.setId(atoi(argv[i]));
		}
		else if (std::string(azColName[i]) == "NAME") {
			user.setName(argv[i]);
		}
	}

	this->m_users.push_back(user);
	return 0;
}


int DatabaseAccess::albumsCallback(void* data, int argc, char** argv, char** azColName)
{
	Album album;

	for (int i = 0; i < argc; i++) {
		if (std::string(azColName[i]) == "NAME") {
			album.setName(argv[i]);
		}
		else if (std::string(azColName[i]) == "CREATION_DATE") {
			album.setCreationDate(argv[i]);
		}
		else if (std::string(azColName[i]) == "USER_ID") {
			album.setOwner(atoi(argv[i]));
		}
		else if (std::string(azColName[i]) == "ID") {
			album.setId(atoi(argv[i]));
		}
	}

	this->m_albums.push_back(album);
	return 0;
}


int DatabaseAccess::picturesCallback(void* data, int argc, char** argv, char** azColName)
{
	unsigned albumId = 0;
	Picture pic;

	for (int i = 0; i < argc; i++) {
		if (std::string(azColName[i]) == "NAME") {
			pic.setName(argv[i]);
		}
		else if (std::string(azColName[i]) == "CREATION_DATE") {
			pic.setCreationDate(argv[i]);
		}
		else if (std::string(azColName[i]) == "ALBUM_ID") {
			albumId = atoi(argv[i]);
		}
		else if (std::string(azColName[i]) == "ID") {
			pic.setId(atoi(argv[i]));
		}
		else if (std::string(azColName[i]) == "LOCATION") {
			pic.setPath(argv[i]);
		}
	}

	try
	{
		auto result = getAlbumById(albumId);
		result.addPicture(pic);
	}

	catch (std::exception& e)
	{
		throw e;
	}

	return 0;
}


int DatabaseAccess::tagsCallback(void* data, int argc, char** argv, char** azColName)
{
	int userId = 0;
	int picId = 0;

	for (unsigned int i = 0; i < argc; i++) {
		if (std::string(azColName[i]) == "USER_ID") {
			userId = atoi(argv[i]);
		}
		
		else if (std::string(azColName[i]) == "PICTURE_ID") {
			picId = atoi(argv[i]);
		}
	}

	// finding the picture in all the albums that the user owns
	for (auto& currAlbum : getAlbumsOfUser(getUser(userId)))
	{
		for (auto& currPic : currAlbum.getPictures())
		{
			if (currPic.getId() == picId)
			{
				currAlbum.tagUserInPicture(userId, currPic.getName());
			}
			break;
		}
	}

	return 0;
}

Album DatabaseAccess::getAlbumById(const int albumId)
{
	for (auto& currAlbum : m_albums) {
		if (currAlbum.getId() == albumId) {
			return currAlbum;
		}
	}

	throw ItemNotFoundException("Album", albumId);
}
