#include <stdio.h>      // standard I/O functions (fopen, fgetc, fwrite, etc.)
#include <stdlib.h>     // memory allocation (malloc, free), exit
#include <string.h>     // string manipulation (strcspn, memset)

// Structure to store frequency of each byte (0-255)
struct frequency {
    unsigned char symbol;  // the byte value
    unsigned int freq;     // frequency count
} frequencies[256];

// Structure for symbols that actually appear in the file
struct used_symbols {
    unsigned char symbol;  // symbol
    unsigned int freq;     // frequency
} used[256];

// Structure to store Huffman codes for each symbol
struct code_generate {
    unsigned char symbol;  // the byte value
    int code[256];         // array storing bits of the Huffman code
    int length;            // length of the Huffman code
} code_generate[256];

// Bit buffer structure used to write bits to a file
struct BitBuffer {
    unsigned char buffer;  // stores bits until we have a full byte
    int bits_filled;       // number of bits currently in buffer
};

// Structure for a node in Huffman tree
struct Huffman_tree {
    unsigned char symbol;         // the byte value (0 if internal node)
    unsigned int freq;            // frequency of the symbol
    struct Huffman_tree *left_branch;  // pointer to left child
    struct Huffman_tree *right_branch; // pointer to right child
};

// Allocate and create a new Huffman tree node
struct Huffman_tree *createNode(unsigned char symbol,
                                unsigned int freq,
                                struct Huffman_tree *left,
                                struct Huffman_tree *right) {
    struct Huffman_tree *node = malloc(sizeof(struct Huffman_tree));  // allocate memory
    node->symbol = symbol;          // set symbol
    node->freq = freq;              // set frequency
    node->left_branch = left;       // set left child
    node->right_branch = right;     // set right child
    return node;                    // return pointer to new node
}

// Build Huffman tree from used[] array
struct Huffman_tree *buildHuffmanTree(struct used_symbols used[], int count) {
    struct Huffman_tree *nodes[512];  // array of pointers to nodes
    int n = count;                    // number of nodes currently in array

    // Create leaf nodes for each symbol
    for (int i = 0; i < count; i++) {
        nodes[i] = createNode(used[i].symbol, used[i].freq, NULL, NULL);
    }

    // Build tree by combining lowest-frequency nodes
    while (n > 1) {
        struct Huffman_tree *left = nodes[0];   // node with lowest frequency
        struct Huffman_tree *right = nodes[1];  // node with second lowest

        // Create parent node combining left and right
        struct Huffman_tree *parent =
            createNode(0, left->freq + right->freq, left, right);

        // Shift remaining nodes left to fill first two positions
        for (int i = 2; i < n; i++) {
            nodes[i - 2] = nodes[i];
        }
        n -= 2;  // remove two nodes combined

        nodes[n] = parent;  // add parent node at the end
        n++;                // total nodes increases by 1

        // Sort nodes by frequency (and symbol if frequencies equal)
        for (int i = 0; i < n - 1; i++) {
            for (int j = i + 1; j < n; j++) {
                if (nodes[i]->freq > nodes[j]->freq ||
                    (nodes[i]->freq == nodes[j]->freq && nodes[i]->symbol > nodes[j]->symbol)) {
                    struct Huffman_tree *tmp = nodes[i];
                    nodes[i] = nodes[j];
                    nodes[j] = tmp;
                }
            }
        }
    }

    return nodes[0];  // return root of tree
}

// Count frequency of each byte in file
void charactercounter(FILE *fptr) {
    int c;
    for (int i = 0; i < 256; i++) {   // initialize frequencies
        frequencies[i].freq = 0;
        frequencies[i].symbol = (unsigned char)i;
    }
    while ((c = fgetc(fptr)) != EOF) {  // read file byte by byte
        frequencies[c].freq++;          // increment frequency of this byte
    }
    rewind(fptr);  // reset file pointer to beginning
}

// Build sorted list of symbols that appear in file
int compactsortedlist() {
    int k = 0;
    for (int i = 0; i < 256; i++) {
        if (frequencies[i].freq > 0) {
            used[k].symbol = frequencies[i].symbol;  // save symbol
            used[k].freq = frequencies[i].freq;      // save frequency
            k++;
        }
    }

    // Sort used[] by frequency (then symbol)
    for (int i = 0; i < k - 1; i++) {
        for (int j = i + 1; j < k; j++) {
            if (used[i].freq > used[j].freq ||
                (used[i].freq == used[j].freq && used[i].symbol > used[j].symbol)) {
                struct used_symbols temp = used[i];
                used[i] = used[j];
                used[j] = temp;
            }
        }
    }
    return k;  // return number of used symbols
}

// Generate Huffman codes by traversing tree
void traverse_tree(struct Huffman_tree *root, int *path, int depth) {
    if (root == NULL) return;  // base case
    if (root->left_branch == NULL && root->right_branch == NULL) {  // leaf node
        int s = root->symbol;
        code_generate[s].symbol = s;      // store symbol
        code_generate[s].length = depth;  // store code length
        for (int i = 0; i < depth; i++)
            code_generate[s].code[i] = path[i];  // copy path bits as code
        return;
    }

    // Traverse left branch (add 0 to path)
    path[depth] = 0;
    traverse_tree(root->left_branch, path, depth + 1);

    // Traverse right branch (add 1 to path)
    path[depth] = 1;
    traverse_tree(root->right_branch, path, depth + 1);
}

// Write bits to file using a buffer
void write_bits(FILE *out, struct BitBuffer *bb, int *code, int length) {
    for (int i = 0; i < length; i++) {
        bb->buffer <<= 1;         // shift buffer left
        bb->buffer |= code[i];    // add current bit
        bb->bits_filled++;        // increment bits count

        if (bb->bits_filled == 8) {  // if buffer full, write to file
            fputc(bb->buffer, out);
            bb->buffer = 0;            // reset buffer
            bb->bits_filled = 0;
        }
    }
}

// Flush remaining bits in buffer
void flush_buffer(FILE *out, struct BitBuffer *bb) {
    if (bb->bits_filled > 0) {
        bb->buffer <<= (8 - bb->bits_filled);  // shift remaining bits
        fputc(bb->buffer, out);               // write last byte
        bb->buffer = 0;
        bb->bits_filled = 0;
    }
}

// Compress file using Huffman coding
void compress_file(const char *input_file, const char *output_file) {
    FILE *in = fopen(input_file, "rb");  // open input file
    FILE *out = fopen(output_file, "wb"); // open output file

    if (!in || !out) {  // check files
        printf("Cannot open files!\n");
        return;
    }

    // Write frequency table at the start
    for (int i = 0; i < 256; i++)
        fwrite(&frequencies[i].freq, sizeof(unsigned int), 1, out);

    struct BitBuffer bb = {0, 0};  // initialize bit buffer
    int c;
    while ((c = fgetc(in)) != EOF) {           // read each byte
        write_bits(out, &bb, code_generate[c].code, code_generate[c].length);  // write code
    }

    flush_buffer(out, &bb);  // write remaining bits
    fclose(in);
    fclose(out);
}

// BitReader initialization
struct BitReader {
    FILE *file;
    unsigned char buffer;
    int bits_left;
};

void init_bit_reader(struct BitReader *br, FILE *file) {
    br->file = file;    // set file pointer
    br->buffer = 0;     // empty buffer
    br->bits_left = 0;  // no bits yet
}

// Read single bit from file
int read_bit(struct BitReader *br) {
    if (br->bits_left == 0) {  // refill buffer if empty
        int c = fgetc(br->file);
        if (c == EOF) return -1;  // end of file
        br->buffer = (unsigned char)c;
        br->bits_left = 8;        // 8 bits available
    }
    int bit = (br->buffer >> 7) & 1;  // get most significant bit
    br->buffer <<= 1;                  // shift buffer
    br->bits_left--;                   // decrement bit count
    return bit;
}

// Free memory of Huffman tree
void free_tree(struct Huffman_tree *root) {
    if (!root) return;
    free_tree(root->left_branch);
    free_tree(root->right_branch);
    free(root);
}

// Decompress file using Huffman coding
void decompress_file(const char *input_file, const char *output_file) {
    FILE *in = fopen(input_file, "rb");
    FILE *out = fopen(output_file, "wb");
    if (!in || !out) { printf("Cannot open files!\n"); return; }

    // Read frequency table
    for (int i = 0; i < 256; i++) {
        frequencies[i].symbol = (unsigned char)i;
        fread(&frequencies[i].freq, sizeof(unsigned int), 1, in);
    }

    // Rebuild used[] list and count total symbols
    unsigned long total_symbols = 0;
    int used_count = 0;
    for (int i = 0; i < 256; i++) {
        if (frequencies[i].freq > 0) {
            used[used_count].symbol = frequencies[i].symbol;
            used[used_count].freq = frequencies[i].freq;
            total_symbols += frequencies[i].freq;  // sum of all bytes
            used_count++;
        }
    }

    // Build Huffman tree
    struct Huffman_tree *root = buildHuffmanTree(used, used_count);

    struct BitReader br;
    init_bit_reader(&br, in);  // initialize bit reader

    struct Huffman_tree *current = root;
    unsigned long symbols_written = 0;

    // Read bits and traverse tree to decode symbols
    while (symbols_written < total_symbols) {
        int bit = read_bit(&br);
        if (bit == -1) break;

        if (bit == 0) current = current->left_branch;
        else current = current->right_branch;

        if (current->left_branch == NULL && current->right_branch == NULL) {
            fputc(current->symbol, out);  // write decoded symbol
            symbols_written++;
            current = root;               // restart from root
        }
    }

    fclose(in);
    fclose(out);
    free_tree(root);  // free memory
}

// Main function: menu, compress or decompress
int main() {
    char filename[256], outputfilename[512];
    int choice;

    printf("Huffman File Compressor/Decompressor\n");
    printf("1. Compress a file\n2. Decompress a file\nEnter your choice (1 or 2): ");
    scanf("%d", &choice);
    getchar();  // consume newline

    printf("Enter the filename: ");
    fgets(filename, sizeof(filename), stdin);
    filename[strcspn(filename, "\n")] = '\0';  // remove newline

    if (choice == 1) {  // compression
        FILE *fptr = fopen(filename, "rb");
        if (!fptr) { printf("Cannot open %s\n", filename); return 1; }

        charactercounter(fptr);         // count symbol frequencies
        int count = compactsortedlist(); // build used[] sorted list
        struct Huffman_tree *root = buildHuffmanTree(used, count); // tree
        int path[256];
        traverse_tree(root, path, 0);   // generate codes

        snprintf(outputfilename, sizeof(outputfilename), "%s.huff", filename); // output filename
        compress_file(filename, outputfilename);  // compress file

        fclose(fptr);
        printf("Compression done! Output file: %s\n", outputfilename);
        free_tree(root);
    } 
    else if (choice == 2) {  // decompression
        snprintf(outputfilename, sizeof(outputfilename), "%s_dec.txt", filename);
        decompress_file(filename, outputfilename);  // decompress file
    } 
    else {
        printf("Invalid choice!\n");
    }
    return 0;
}
