#pragma once
#include <list>
#include "Album.h"
#include "User.h"
#include "IDataAccess.h"
#include <stdio.h>

class DatabaseAccess : public IDataAccess
{

public:
	DatabaseAccess();
	virtual ~DatabaseAccess() = default;

	// album related
	const std::list<Album> getAlbums() override;
	std::list<Album> getAlbumsOfUser(const User& user) override;
	void createAlbum(const Album& album) override;
	void deleteAlbum(const std::string& albumName, int userId) override;
	bool doesAlbumExists(const std::string& albumName, int userId) override;
	Album openAlbum(const std::string& albumName) override;
	Album getAlbumById(const int albumId) override;
	void closeAlbum(Album& pAlbum) override;
	void printAlbums() override;

	// picture related
	void addPictureToAlbumByName(const std::string& albumName, const Picture& picture) override;
	void removePictureFromAlbumByName(const std::string& albumName, const std::string& pictureName) override;
	void tagUserInPicture(const std::string& albumName, const std::string& pictureName, int userId) override;
	void untagUserInPicture(const std::string& albumName, const std::string& pictureName, int userId) override;

	// user related
	void printUsers() override;
	void createUser(User& user) override;
	void deleteUser(const User& user) override;
	bool doesUserExists(int userId) override;
	User getUser(int userId) override;
	void deleteUsersAlbums(const User& user) override;
	void deleteUserTags(const User& user) override;

	// user statistics
	int countAlbumsOwnedOfUser(const User& user) override;
	int countAlbumsTaggedOfUser(const User& user) override;
	int countTagsOfUser(const User& user) override;
	float averageTagsPerAlbumOfUser(const User& user) override;

	// queries
	User getTopTaggedUser() override;
	Picture getTopTaggedPicture() override;
	std::list<Picture> getTaggedPicturesOfUser(const User& user) override;

	// callback functions
	int usersCallback(void* data, int argc, char** argv, char** azColName) override;
	int albumsCallback(void* data, int argc, char** argv, char** azColName) override;
	int picturesCallback(void* data, int argc, char** argv, char** azColName) override;
	int tagsCallback(void* data, int argc, char** argv, char** azColName) override;

	bool open() override;
	void close() override;
	void clear() override;
	bool runSqlCommand(std::string sqlStatement) override;
	void dropTables() override;

private:
	std::list<Album> m_albums;
	std::list<User> m_users;
	sqlite3* db;
	std::string dbFileName;

	auto getAlbumIfExists(const std::string& albumName);
	//void cleanUserData(const User& userId);
};
