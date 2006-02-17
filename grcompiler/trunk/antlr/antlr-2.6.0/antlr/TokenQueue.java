package antlr;

/**
 * <b>SOFTWARE RIGHTS</b>
 * <p>
 * ANTLR 2.6.0 MageLang Institute
 * <p>
 * We reserve no legal rights to the ANTLR--it is fully in the
 * public domain. An individual or company may do whatever
 * they wish with source code distributed with ANTLR or the
 * code generated by ANTLR, including the incorporation of
 * ANTLR, or its output, into commerical software.
 * <p>
 * We encourage users to develop software with ANTLR. However,
 * we do ask that credit is given to us for developing
 * ANTLR. By "credit", we mean that if you use ANTLR or
 * incorporate any source code into one of your programs
 * (commercial product, research project, or otherwise) that
 * you acknowledge this fact somewhere in the documentation,
 * research report, etc... If you like ANTLR and have
 * developed a nice tool with the output, please mention that
 * you developed it using ANTLR. In addition, we ask that the
 * headers remain intact in our source code. As long as these
 * guidelines are kept, we expect to continue enhancing this
 * system and expect to make other tools available as they are
 * completed.
 * <p>
 * The ANTLR gang:
 * @version ANTLR 2.6.0 MageLang Institute
 * @author Terence Parr, <a href=http://www.MageLang.com>MageLang Institute</a>
 * @author <br>John Lilley, <a href=http://www.Empathy.com>Empathy Software</a>
 */
/** A private circular buffer object used by the token buffer */
class TokenQueue {
	// Physical circular buffer of tokens
	private Token[] buffer;
	// buffer.length-1 for quick modulous
	private int sizeLessOne;
	// physical index of front token
	private int offset;
	// number of tokens in the queue
	protected int nbrEntries;


	public TokenQueue(int minSize) {
		// Find first power of 2 >= to requested size
		int size;
		for (size = 2; size < minSize; size *= 2) {;}
		init(size);
	}
	/** Add token to end of the queue
	 * @param tok The token to add
	 */
	public final void append(Token tok)
	{
		if (nbrEntries == buffer.length)
		{
			expand();
		}
		buffer[(offset + nbrEntries) & sizeLessOne] = tok;
		nbrEntries++;
	}
	/** Fetch a token from the queue by index
	 * @param idx The index of the token to fetch, where zero is the token at the front of the queue
	 */
	public final Token elementAt(int idx) { 
		return buffer[(offset + idx) & sizeLessOne];
	}
	/** Expand the token buffer by doubling its capacity */
	private final void expand()
	{
		Token[] newBuffer = new Token[buffer.length * 2];
		// Copy the contents to the new buffer
		// Note that this will store the first logical item in the 
		// first physical array element.
		for (int i = 0; i < buffer.length; i++)
		{
			newBuffer[i] = elementAt(i);
		}
		// Re-initialize with new contents, keep old nbrEntries
		buffer = newBuffer;
		sizeLessOne = buffer.length - 1;
		offset = 0;
	}
	/** Initialize the queue.
	 * @param size The initial size of the queue
	 */
	private final void init(int size) {
		// Allocate buffer
		buffer = new Token[size];
		// Other initialization
		sizeLessOne = size - 1;
		offset = 0;
		nbrEntries = 0;
	}
	/** Remove token from front of queue */
	public final void removeFirst() { 
		offset = (offset+1) & sizeLessOne;
		nbrEntries--;
	}
}