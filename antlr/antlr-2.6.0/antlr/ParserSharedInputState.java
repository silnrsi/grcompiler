package antlr;

/** This object contains the data associated with an
 *  input stream of tokens.  Multiple parsers
 *  share a single ParserSharedInputState to parse
 *  the same stream of tokens.
 */
public class ParserSharedInputState {
	/** Where to get token objects */
	protected TokenBuffer input;
	/** Are we guessing (guessing>0)? */
	public int guessing = 0;
	/** What file (if known) caused the problem? */
	protected String filename;
}