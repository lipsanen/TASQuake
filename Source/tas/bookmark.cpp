#include "bookmark.hpp"

#include <unordered_map>

#include "cpp_quakedef.hpp"
#include "script_playback.hpp"
#include "hooks.h"

enum class BookmarkType {Frame, Block, Map};

struct Bookmark
{
	Bookmark();
	Bookmark(int index, BookmarkType type);

	int index;
	BookmarkType type;
};

Bookmark::Bookmark() : index(0), type(BookmarkType::Frame) { }

Bookmark::Bookmark(int index, BookmarkType type)
{
	this->index = index;
	this->type = type;
}

static std::unordered_map<std::string, Bookmark> bookmarks;


void Clear_Bookmarks()
{
	bookmarks.clear();
}

void Cmd_TAS_Bookmark_Frame(void)
{
	if (Cmd_Argc() == 1)
	{
		Con_Print("Usage: tas_bookmark_frame <bookmark name>\n");
		return;
	}

	auto playback = GetPlaybackInfo();
	std::string name = Cmd_Argv(1);
	bookmarks[name] = Bookmark(playback->current_frame, BookmarkType::Frame);
}

void Cmd_TAS_Bookmark_Block(void)
{
	auto playback = GetPlaybackInfo();
	int current_block = playback->GetBlockNumber();

	if (Cmd_Argc() == 1)
	{
		Con_Print("Usage: tas_bookmark_block <bookmark name>\n");
		return;
	}
	else if (!CurrentFrameHasBlock())
	{
		Con_Print("Current frame has no block.\n");
		return;
	}

	std::string name = Cmd_Argv(1);
	bookmarks[name] = Bookmark(current_block, BookmarkType::Block);
}

void Cmd_TAS_Bookmark_Skip(void)
{
	auto playback = GetPlaybackInfo();

	if (Cmd_Argc() == 1)
	{
		Con_Print("Usage: tas_bookmark_block <bookmark name>\n");
		return;
	}

	std::string name = Cmd_Argv(1);
	if (bookmarks.find(name) == bookmarks.end())
	{
		Con_Printf("Usage: No bookmark with name %s\n", Cmd_Argv(1));
		return;
	}

	auto& bookmark = bookmarks[name];
	if (bookmark.type == BookmarkType::Frame || bookmark.type == BookmarkType::Map)
	{
		Run_Script(bookmark.index, true);
	}
	else
	{
		Skip_To_Block(bookmark.index);
	}
}

void Bookmark_Frame_Hook()
{
	auto playback = GetPlaybackInfo();
	if (cl.movemessages == 0 && playback->current_frame > 0 && tas_playing.value != 0 && tas_gamestate == unpaused && cls.signon == SIGNONS)
	{
		std::string map = sv.name;
		bookmarks[map] = Bookmark(playback->current_frame, BookmarkType::Map);
	}
}
