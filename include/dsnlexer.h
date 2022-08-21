/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2007-2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2007-2021 Kicad Developers, see change_log.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef DSNLEXER_H_
#define DSNLEXER_H_

#include <cstdio>
#include <hashtables.h>
#include <string>
#include <vector>

#include <richio.h>

#ifndef SWIG
/**
 * Hold a keyword string and its unique integer token.
 */
struct KEYWORD
{
    const char* name;       ///< unique keyword.
    int         token;      ///< a zero based index into an array of KEYWORDs
};
#endif // SWIG

// something like this macro can be used to help initialize a KEYWORD table.
// see SPECCTRA_DB::keywords[] as an example.

//#define TOKDEF(x)    { #x, T_##x }


/**
 * List all the DSN lexer's tokens that are supported in lexing.
 *
 * It is up to the parser if it wants also to support them.
 */
enum DSN_SYNTAX_T
{
    DSN_NONE         = -11,
    DSN_COMMENT      = -10,
    DSN_STRING_QUOTE = -9,
    DSN_QUOTE_DEF    = -8,
    DSN_DASH         = -7,
    DSN_SYMBOL       = -6,
    DSN_NUMBER       = -5,
    DSN_RIGHT        = -4,  // right bracket, ')'
    DSN_LEFT         = -3,  // left bracket, '('
    DSN_STRING       = -2,  // a quoted string, stripped of the quotes
    DSN_EOF          = -1   // special case for end of file
};


/**
 * Implement a lexical analyzer for the SPECCTRA DSN file format.
 *
 * It reads lexical tokens from the current #LINE_READER through the #NextTok() function.
 */
class DSNLEXER
{
public:

    /**
     * Initialize a DSN lexer and prepares to read from aFile which is already open and has
     * \a aFilename.
     *
     * @param aKeywordTable is an array of KEYWORDS holding \a aKeywordCount.  This
     *  token table need not contain the lexer separators such as '(' ')', etc.
     * @param aKeywordCount is the count of tokens in aKeywordTable.
     * @param aFile is an open file, which will be closed when this is destructed.
     * @param aFileName is the name of the file
     */
    DSNLEXER( const KEYWORD* aKeywordTable, unsigned aKeywordCount, const KEYWORD_MAP* aKeywordMap,
              FILE* aFile, const wxString& aFileName );

    /**
     * Initialize a DSN lexer and prepares to read from @a aSExpression.
     *
     * @param aKeywordTable is an array of KEYWORDS holding \a aKeywordCount.  This
     *  token table need not contain the lexer separators such as '(' ')', etc.
     * @param aKeywordCount is the count of tokens in aKeywordTable.
     * @param aSExpression is text to feed through a STRING_LINE_READER
     * @param aSource is a description of aSExpression, used for error reporting.
     */
    DSNLEXER( const KEYWORD* aKeywordTable, unsigned aKeywordCount, const KEYWORD_MAP* aKeywordMap,
              const std::string& aSExpression, const wxString& aSource = wxEmptyString );

    /**
     * Initialize a DSN lexer and prepares to read from @a aSExpression.
     *
     * Use this one without a keyword table with the DOM parser in ptree.h.
     *
     * @param aSExpression is text to feed through a #STRING_LINE_READER
     * @param aSource is a description of aSExpression, used for error reporting.
     */
    DSNLEXER( const std::string& aSExpression, const wxString& aSource = wxEmptyString );

    /**
     * Initialize a DSN lexer and prepares to read from @a aLineReader which is already
     * open, and may be in use by other DSNLEXERs also.
     *
     * No ownership is taken of @a aLineReader. This enables it to be used by other DSNLEXERs.
     *
     * @param aKeywordTable is an array of #KEYWORDS holding \a aKeywordCount.  This
     *  token table need not contain the lexer separators such as '(' ')', etc.
     * @param aKeywordCount is the count of tokens in aKeywordTable.
     * @param aLineReader is any subclassed instance of LINE_READER, such as
     *  #STRING_LINE_READER or #FILE_LINE_READER.  No ownership is taken.
     */
    DSNLEXER( const KEYWORD* aKeywordTable, unsigned aKeywordCount, const KEYWORD_MAP* aKeywordMap,
              LINE_READER* aLineReader = nullptr );

    virtual ~DSNLEXER();

    /**
     * Reinit variables used during parsing, to ensure od states are not used in a new parsing
     * must be called before parsing a new file after parsing an old file to avoid
     * starting with some variables in a non initial state
     */
    void InitParserState();

    /**
     * Usable only for DSN lexers which share the same #LINE_READER.
     *
     * Synchronizes the pointers handling the data read by the #LINE_READER.  Allows 2
     * #DNSLEXER objects to share the same current line, when switching from a #DNSLEXER
     * to another #DNSLEXER
     * @param aLexer the model.
     * @return true if the sync can be made ( at least the same line reader ).
     */
    bool SyncLineReaderWith( DSNLEXER& aLexer );

    /**
     * Change the behavior of this lexer into or out of "specctra mode".
     *
     * If specctra mode, then:
     *  -#) stringDelimiter can be changed.
     *  -#) KiCad quoting protocol is not in effect.
     *  -#) space_in_quoted_tokens is functional else none of the above are true.
     *
     * The default mode is non-specctra mode, meaning:
     *  -#) stringDelimiter cannot be changed.
     *  -#) KiCad quoting protocol is in effect.
     *  -#) space_in_quoted_tokens is not functional.
     */
    void SetSpecctraMode( bool aMode );

    /**
     * Manage a stack of LINE_READERs in order to handle nested file inclusion.
     *
     * This function pushes aLineReader onto the top of a stack of LINE_READERs and makes
     * it the current #LINE_READER with its own #GetSource(), line number and line text.
     * A grammar must be designed such that the "include" token (whatever its various names),
     * and any of its parameters are not followed by anything on that same line,
     * because PopReader always starts reading from a new line upon returning to
     * the original #LINE_READER.
     */
    void PushReader( LINE_READER* aLineReader );

    /**
     * Delete the top most #LINE_READER from an internal stack of LINE_READERs and
     * in the case of #FILE_LINE_READER this means the associated FILE is closed.
     *
     * The most recently used former #LINE_READER on the stack becomes the
     * current #LINE_READER and its previous position in its input stream and the
     * its latest line number should pertain.  PopReader always starts reading
     * from a new line upon returning to the previous #LINE_READER.  A pop is only
     * possible if there are at least 2 #LINE_READERs on the stack, since popping
     * the last one is not supported.
     *
     * @return the LINE_READER that was in use before the pop, or NULL
     *   if there was not at least two readers on the stack and therefore the
     *   pop failed.
     */
    LINE_READER* PopReader();

    /**
     * Return the next token found in the input file or DSN_EOF when reaching the end of
     * file.
     *
     * Users should wrap this function to return an enum to aid in grammar debugging while
     * running under a debugger, but leave this lower level function returning an int (so
     * the enum does not collide with another usage).
     *
     * @return the type of token found next.
     * @throw IO_ERROR only if the #LINE_READER throws it.
     */
    int NextTok();

    /**
     * Call #NextTok() and then verifies that the token read in satisfies #IsSymbol().
     *
     * @return the actual token read in.
     * @throw IO_ERROR if the next token does not satisfy IsSymbol().
     */
    int NeedSYMBOL();

    /**
     * Call #NextTok() and then verifies that the token read in satisfies bool IsSymbol() or
     * the next token is #DSN_NUMBER.
     *
     * @return the actual token read in.
     * @throw IO_ERROR if the next token does not satisfy the above test.
     */
    int NeedSYMBOLorNUMBER();

    /**
     * Call #NextTok() and then verifies that the token read is type #DSN_NUMBER.
     *
     * @return the actual token read in.
     * @throw IO_ERROR if the next token does not satisfy the above test.
     */
    int NeedNUMBER( const char* aExpectation );

    /**
     * Return whatever #NextTok() returned the last time it was called.
     */
    int CurTok() const
    {
        return curTok;
    }

    /**
     * Return whatever NextTok() returned the 2nd to last time it was called.
     */
    int PrevTok() const
    {
        return prevTok;
    }

    /**
     * Used to support "loose" matches (quoted tokens).
     */
    int GetCurStrAsToken() const
    {
        return findToken( curText );
    }

    /**
     * Change the string delimiter from the default " to some other character and return
     * the old value.
     *
     * @param aStringDelimiter The character in lowest 8 bits.
     * @return The old delimiter in the lowest 8 bits.
     */
    char SetStringDelimiter( char aStringDelimiter )
    {
        int old = stringDelimiter;

        if( specctraMode )
            stringDelimiter = aStringDelimiter;

        return old;
    }

    /**
     * Change the setting controlling whether a space in a quoted string isa terminator.
     *
     * @param val If true, means
     */
    bool SetSpaceInQuotedTokens( bool val )
    {
        bool old = space_in_quoted_tokens;

        if( specctraMode )
            space_in_quoted_tokens = val;

        return old;
    }

    /**
     * Change the handling of comments.
     *
     * If set true, comments are returned as single line strings with a terminating newline.
     * Otherwise they are consumed by the lexer and not returned.
     */
    bool SetCommentsAreTokens( bool val )
    {
        bool old = commentsAreTokens;
        commentsAreTokens = val;
        return old;
    }

    /**
     * Check the next sequence of tokens and reads them into a wxArrayString if they are
     * comments.
     *
     * Reading continues until a non-comment token is encountered, and such last read token
     * remains as #CurTok() and as #CurText().  No push back or "un get" mechanism is used
     * for this support.  Upon return you simply avoid calling NextTok() for the next token,
     * but rather #CurTok().
     *
     * @return Heap allocated block of comments or NULL if none.  The caller owns the
     *         allocation and must delete if not NULL.
     */
    wxArrayString* ReadCommentLines();

    /**
     * Test a token to see if it is a symbol.
     *
     * This means it cannot be a special delimiter character such as #DSN_LEFT, #DSN_RIGHT,
     * #DSN_QUOTE, etc.  It may however, coincidentally match a keyword and still be a symbol.
     */
    static bool IsSymbol( int aTok );

    /**
     * Throw an #IO_ERROR exception with an input file specific error message.
     *
     * @param aTok is the token/keyword type which was expected at the current input location.
     * @throw IO_ERROR with the location within the input file of the problem.
     */
    void Expecting( int aTok ) const;

    /**
     * Throw an #IO_ERROR exception with an input file specific error message.
     *
     * @param aTokenList is the token/keyword type which was expected at the
     *                   current input location, e.g.  "pin|graphic|property".
     * @throw IO_ERROR with the location within the input file of the problem.
     */
    void Expecting( const char* aTokenList ) const;

    /**
     * Throw an #IO_ERROR exception with an input file specific error message.
     *
     * @param aTok is the token/keyword type which was not expected at the
     *             current input location.
     * @throw IO_ERROR with the location within the input file of the problem.
     */
    void Unexpected( int aTok ) const;

    /**
     * Throw an #IO_ERROR exception with an input file specific error message.
     *
     * @param aToken is the token which was not expected at the current input location.
     * @throw IO_ERROR with the location within the input file of the problem.
     */
    void Unexpected( const char* aToken ) const;

    /**
     * Throw an #IO_ERROR exception with a message saying specifically that \a aTok
     * is a duplicate of one already seen in current context.
     *
     * @param aTok is the token/keyword type which was not expected at the current input
     *             location.
     * @throw IO_ERROR with the location within the input file of the problem.
     */
    void Duplicate( int aTok );

    /**
     * Call #NextTok() and then verifies that the token read in is a #DSN_LEFT.
     *
     * @throw IO_ERROR if the next token is not a #DSN_LEFT
     */
    void NeedLEFT();

    /**
     * Call #NextTok() and then verifies that the token read in is a #DSN_RIGHT.
     *
     * @throw IO_ERROR if the next token is not a #DSN_RIGHT
     */
    void NeedRIGHT();

    /**
     * Return the C string representation of a #DSN_T value.
     */
    const char* GetTokenText( int aTok ) const;

    /**
     * Return a quote wrapped wxString representation of a token value.
     */
    wxString GetTokenString( int aTok ) const;

    static const char* Syntax( int aTok );

    /**
     * Return a pointer to the current token's text.
     */
    const char* CurText() const
    {
        return curText.c_str();
    }

    /**
     * Return a reference to current token in std::string form.
     */
    const std::string& CurStr() const
    {
        return curText;
    }

    /**
     * Return the current token text as a wxString, assuming that the input byte stream
     * is UTF8 encoded.
     */
    wxString FromUTF8() const
    {
        return wxString::FromUTF8( curText.c_str() );
    }

    /**
     * Return the current line number within my #LINE_READER.
     */
    int CurLineNumber() const
    {
        return reader->LineNumber();
    }

    /**
     * Return the current line of text from which the #CurText() would return its token.
     */
    const char* CurLine() const
    {
        return (const char*)(*reader);
    }

    /**
     * Return the current #LINE_READER source.
     *
     * @return source of the lines of text, e.g. a filename or "clipboard".
     */
    const wxString& CurSource() const
    {
        return reader->GetSource();
    }

    /**
     * Return the byte offset within the current line, using a 1 based index.
     *
     * @return a one based index into the current line.
     */
    int CurOffset() const
    {
        return curOffset + 1;
    }

#ifndef SWIG

protected:
    void init();

    int readLine()
    {
        if( reader )
        {
            reader->ReadLine();

            unsigned len = reader->Length();

            // start may have changed in ReadLine(), which can resize and
            // relocate reader's line buffer.
            start = reader->Line();

            next  = start;
            limit = next + len;

            return len;
        }
        return 0;
    }

    /**
     * Take @a aToken string and looks up the string in the keywords table.
     *
     * @param aToken is a string to lookup in the keywords table.
     * @return with a value from the enum #DSN_T matching the keyword text,
     *         or #DSN_SYMBOL if @a aToken is not in the keywords table.
     */
    int findToken( const std::string& aToken ) const;

    bool isStringTerminator( char cc ) const
    {
        if( !space_in_quoted_tokens && cc == ' ' )
            return true;

        if( cc == stringDelimiter )
            return true;

        return false;
    }

    /**
     * Parse the current token as an ASCII numeric string with possible leading
     * whitespace into a double precision floating point number.
     *
     * @throw IO_ERROR if an error occurs attempting to convert the current token.
     * @return The result of the parsed token.
     */
    double parseDouble();

    double parseDouble( const char* aExpected )
    {
        NeedNUMBER( aExpected );
        return parseDouble();
    }

    template <typename T>
    inline double parseDouble( T aToken )
    {
        return parseDouble( GetTokenText( aToken ) );
    }

    bool                iOwnReaders;            ///< on readerStack, should I delete them?
    const char*         start;
    const char*         next;
    const char*         limit;
    char                dummy[1];               ///< when there is no reader.

    typedef std::vector<LINE_READER*>  READER_STACK;

    READER_STACK        readerStack;            ///< all the LINE_READERs by pointer.

    ///< no ownership. ownership is via readerStack, maybe, if iOwnReaders
    LINE_READER*        reader;

    bool                specctraMode;           ///< if true, then:
                                                ///< 1) stringDelimiter can be changed
                                                ///< 2) Kicad quoting protocol is not in effect
                                                ///< 3) space_in_quoted_tokens is functional
                                                ///< else not.

    char                stringDelimiter;
    bool                space_in_quoted_tokens; ///< blank spaces within quoted strings

    bool                commentsAreTokens;      ///< true if should return comments as tokens

    int                 prevTok;                ///< curTok from previous NextTok() call.
    int                 curOffset;              ///< offset within current line of the current token

    int                 curTok;                 ///< the current token obtained on last NextTok()
    std::string         curText;                ///< the text of the current token

    const KEYWORD*      keywords;               ///< table sorted by CMake for bsearch()
    unsigned            keywordCount;           ///< count of keywords table
    const KEYWORD_MAP*  keywordsLookup;         ///< fast, specialized "C string" hashtable
#endif // SWIG
};

template<typename ENUM_TYPE>
class DSNLEXER_KEYWORDED : public DSNLEXER
{
public:
    /**
     * Constructor ( const std::string&, const wxString& )
     * @param aSExpression is (utf8) text possibly from the clipboard that you want to parse.
     * @param aSource is a description of the origin of @a aSExpression, such as a filename.
     *   If left empty, then _("clipboard") is used.
     */
    DSNLEXER_KEYWORDED( const std::string& aSExpression, const wxString& aSource = wxEmptyString ) :
            DSNLEXER( keywords, keyword_count, &keywords_hash, aSExpression, aSource )
    {
    }

    /**
     * Constructor ( FILE* )
     * takes @a aFile already opened for reading and @a aFilename as parameters.
     * The opened file is assumed to be positioned at the beginning of the file
     * for purposes of accurate line number reporting in error messages.  The
     * FILE is closed by this instance when its destructor is called.
     * @param aFile is a FILE already opened for reading.
     * @param aFilename is the name of the opened file, needed for error reporting.
     */
    DSNLEXER_KEYWORDED( FILE* aFile, const wxString& aFilename ) :
            DSNLEXER( keywords, keyword_count, &keywords_hash, aFile, aFilename )
    {
    }

    /**
     * Constructor ( LINE_READER* )
     * initializes a lexer and prepares to read from @a aLineReader which
     * is assumed ready, and may be in use by other DSNLEXERs also.  No ownership
     * is taken of @a aLineReader. This enables it to be used by other lexers also.
     * The transition between grammars in such a case, must happen on a text
     * line boundary, not within the same line of text.
     *
     * @param aLineReader is any subclassed instance of LINE_READER, such as
     *  STRING_LINE_READER or FILE_LINE_READER.  No ownership is taken of aLineReader.
     */
    DSNLEXER_KEYWORDED( LINE_READER* aLineReader ) :
            DSNLEXER( keywords, keyword_count, &keywords_hash, aLineReader )
    {
    }


    /**
     * Function NextTok
     * returns the next token found in the input file or T_EOF when reaching
     * the end of file.  Users should wrap this function to return an enum
     * to aid in grammar debugging while running under a debugger, but leave
     * this lower level function returning an int (so the enum does not collide
     * with another usage).
     * @return TSCHEMATIC_T::T - the type of token found next.
     * @throw IO_ERROR - only if the LINE_READER throws it.
     */
    ENUM_TYPE NextTok()
    {
        return (ENUM_TYPE) DSNLEXER::NextTok();
    }

    /**
     * Function NeedSYMBOL
     * calls NextTok() and then verifies that the token read in
     * satisfies bool IsSymbol().
     * If not, an IO_ERROR is thrown.
     * @return int - the actual token read in.
     * @throw IO_ERROR, if the next token does not satisfy IsSymbol()
     */
    ENUM_TYPE NeedSYMBOL()
    {
        return (ENUM_TYPE) DSNLEXER::NeedSYMBOL();
    }

    /**
     * Function NeedSYMBOLorNUMBER
     * calls NextTok() and then verifies that the token read in
     * satisfies bool IsSymbol() or tok==T_NUMBER.
     * If not, an IO_ERROR is thrown.
     * @return int - the actual token read in.
     * @throw IO_ERROR, if the next token does not satisfy the above test
     */
    ENUM_TYPE NeedSYMBOLorNUMBER()
    {
        return (ENUM_TYPE) DSNLEXER::NeedSYMBOLorNUMBER();
    }

    /**
     * Function CurTok
     * returns whatever NextTok() returned the last time it was called.
     */
    ENUM_TYPE CurTok()
    {
        return (ENUM_TYPE) DSNLEXER::CurTok();
    }

    /**
     * Function PrevTok
     * returns whatever NextTok() returned the 2nd to last time it was called.
     */
    ENUM_TYPE PrevTok()
    {
        return (ENUM_TYPE) DSNLEXER::PrevTok();
    }

    /**
     * Function GetCurStrAsToken
     * Used to support 'loose' matches (quoted tokens)
     */
    ENUM_TYPE GetCurStrAsToken()
    {
        return (ENUM_TYPE) DSNLEXER::GetCurStrAsToken();
    }

    /**
     * Function TokenName
     * returns the name of the token in ASCII form.
     */
    static const char* TokenName( ENUM_TYPE aTok );

protected:
    /// Auto generated lexer keywords table and length:
    static const KEYWORD     keywords[];
    static const KEYWORD_MAP keywords_hash;
    static const unsigned    keyword_count;
};


template <typename ENUM_TYPE>
const char* DSNLEXER_KEYWORDED <ENUM_TYPE>::TokenName( ENUM_TYPE aTok )
{
    const char* ret;

    if( aTok < 0 )
        ret = DSNLEXER::Syntax( aTok );
    else if( (unsigned) aTok < keyword_count )
        ret = keywords[aTok].name;
    else
        ret = "token too big";

    return ret;
}

#endif  // DSNLEXER_H_
