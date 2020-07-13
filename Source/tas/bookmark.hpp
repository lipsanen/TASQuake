void Clear_Bookmarks();
// desc: Usage: tas_bookmark_frame <name>. Bookmarks the current frame with the name given as the argument.
void Cmd_TAS_Bookmark_Frame(void);
// desc: Usage: tas_bookmark_block <name>. Bookmarks the current block with the name given as the argument.
void Cmd_TAS_Bookmark_Block(void);
// desc: Usage: tas_bookmark_skip <name>. Skips to bookmark with the given name.
void Cmd_TAS_Bookmark_Skip(void);

void Bookmark_Frame_Hook();