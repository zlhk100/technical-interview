#include <iostream>
#include <string>
#include <fstream>
#include <cstring>
#include <set>
#include <vector>
#include <assert.h>

using namespace std;

class TextFileParser {
public:
    static TextFileParser* createParser(int, int, string&, string&);
    ~TextFileParser();

    int loadChunkFileIntoBuffer();
    int preprocessBuffer();
    int searchForSentences();

    int process();

private:
    TextFileParser(int min, int max, string& in, string& out) :
                       mMin(min), mMax(max), mInFile(in), mOutFile(out), mSeekPtr(0), mActualSize(0) { }

    int writeCandidates2File();
    int insert2Set(int first, int last);
    struct Sentence {
        int startPtr;
	int endPtr;
	int length;
	Sentence() : startPtr(0), endPtr(0), length(0) { }
    };
    

private:
    static const int kRawBufferSize = 4096;
    string mInFile, mOutFile;
    ifstream mInReader;
    ofstream mOutWriter;
    int mMin, mMax;

    char *mRawBuffer; // load one chunk of file into buffer
    long mSeekPtr;               // track seekPtr of next loading.  considering sentence delimiter.
    size_t  mActualSize;            // actual size of content loaded into buffer.

    vector<Sentence> mSentences; // preprocess raw text buffer in-place eg. replace all special char into blanks
                                 // cache info for each valid sentences.
    set<string> mCandidates;     // sorted candidate sentence or sequence sentence.
};

int TextFileParser::loadChunkFileIntoBuffer() {
    // read fixed size of file into raw buffer, return 0 in case error or file if empty otherwise actual size read into.
    // start from mSeekPtr.
    if (mInReader.good()) {
	    mInReader.seekg(mSeekPtr, ios::beg);
	    mInReader.read(mRawBuffer, kRawBufferSize);

	    mActualSize = mInReader.gcount();
    } else {
            mActualSize = 0;
    }

    return mActualSize;
}

int TextFileParser::preprocessBuffer() {
    // scan through the raw buffer, replace in place all special chars.
    // index each valid sentence in this process and extract sentence meta data into database
    // identify mSeekPtr of next chunk of file content to be read. consider that sentence delimitor goes across buffer.
    int ret = 0; // no error

    Sentence sentence;  
    int i = 0;
    for (; i < mActualSize; i++) {
 
        if (mRawBuffer[i] == '\0' || mRawBuffer[i] == '\r' || mRawBuffer[i] == '\n')
	    mRawBuffer[i] = ' ';


        if (mRawBuffer[i] == '?' || mRawBuffer[i] == '.' || mRawBuffer[i] == '!')  { // delimiter is found.
            // Sentence sentence; 
	    sentence.length = i - sentence.startPtr + 1;
            sentence.endPtr = i;
	    mSentences.push_back(sentence);
	    sentence.startPtr = i + 1;
	}
    }
    
    if ( sentence.endPtr == i - 1)
        mSeekPtr += mActualSize;
    else
        mSeekPtr += mActualSize - (i - sentence.startPtr);
    
    return ret;
}

int TextFileParser::writeCandidates2File() {
    int ret = 0;

    for (auto& it : mCandidates) {
        if (mOutWriter.good()) {
            string value = it + '\n';
	    mOutWriter.write(value.data(), value.size());
	}
    }

    return ret;
}

int TextFileParser::insert2Set(int first, int last) {
    assert(first <= last);

    int ret = 0;
    string value = "";
    for (int i = first; i <= last; i++) {
        value += string(mRawBuffer + mSentences[i].startPtr, mSentences[i].length);
    }
    mCandidates.insert(value);

    return ret;
}

int TextFileParser::searchForSentences() {
    // traverse through sentence indexing vector<> DB and search for valid sentence candidates.
    // once identified, output to file.
    size_t first, last, total;
    first = last = total = 0;
    int ret = 0;

    for (int i = 0; i < mSentences.size(); i++) {

        total += mSentences[i].length;
	last = i;
        
	if (total > mMax) {
            first = i;
	    total = mSentences[i].length;
	}

	if (total >= mMin && total <= mMax) {
	    insert2Set(first, last);
	    total = 0;
	    first = last = i + 1;

	} else if (total > mMax) {
            total = 0;
	    first = last = i + 1;
	}
    }

    return ret;
}

int TextFileParser::process() {
    int hasMore = 0;

    // load file into buffer chunk by chunk
    while (hasMore = loadChunkFileIntoBuffer()) {

        // clear index vector for new cunks of files
        mSentences.clear();

	preprocessBuffer();

	searchForSentences();

    }

    // write set<> to file.
    writeCandidates2File();

    return 0;
}

TextFileParser::~TextFileParser() {
    mOutWriter.close();
    mInReader.close();
}

TextFileParser* TextFileParser::createParser(int min, int max, string& in, string& out) {
    TextFileParser* self = new TextFileParser(min, max, in, out);
    if (!self)
        return NULL;
   
    self->mInReader.open(self->mInFile.c_str(), std::ios::binary);
    if (!self->mInReader.good()) {
        goto INFILE_FAIL;
    }

    self->mOutWriter.open(self->mOutFile.c_str(), ios::binary | ios::out);
    if (!self->mOutWriter.good()) {
        goto OUTFILE_FAIL;
    }

    self->mRawBuffer = new char[kRawBufferSize];
    if (!self->mRawBuffer) {
        goto RAW_BUF_FAIL;
    }

    return self;

RAW_BUF_FAIL:
    self->mOutWriter.close();
OUTFILE_FAIL:
    self->mInReader.close();
INFILE_FAIL:
    delete self;
    return NULL;
}

int main(int argc, char* argv[])
{
    if (argc != 5) {
        std::cerr << "please type prog.exe min max in_file our_file." << endl;
        return 1;
    }
    
    size_t min = atoi(argv[1]), max = atoi(argv[2]);

    if ( argv[1] > argv[2] || min > 1000 || max > 1000 ) {
        std::cerr << "make sure proper max >= min" << endl;
	return 1;
    }

    string infile(argv[3]);
    string outfile(argv[4]);
    TextFileParser *parser = TextFileParser::createParser(min, max, infile, outfile);
    if (!parser) {
        std::cerr << "Failed to create Text parser." << endl;
        return 1;
    }
    
    parser->process();

    delete parser;

    return 0;
}


    


