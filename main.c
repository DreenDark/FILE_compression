#include<stdio.h>
#include<stdlib.h>
#include<string.h>

struct frequency
{
  unsigned char symbol;
  unsigned int freq; 
}frequencies[256];

struct used_symbols{
    unsigned char symbol;
    unsigned int freq;
}used[256];

struct code_generate{
    unsigned char symbol;
    int code[256];
    int length;
}code_generate[256];

struct BitBuffer {
    unsigned char buffer; 
    int bits_filled; 
};

struct Huffman_tree{
    unsigned char symbol;
    unsigned int freq;
    struct Huffman_tree *right_branch;
    struct Huffman_tree *left_branch;
};

struct Huffman_tree *createNode(unsigned char symbol,unsigned int freq,struct Huffman_tree *left,struct Huffman_tree *right){
    struct Huffman_tree *node = malloc(sizeof(struct Huffman_tree));
    node->symbol = symbol;
    node->freq = freq;
    node->left_branch = left;
    node->right_branch = right;
    return node;
}

struct Huffman_tree *buildHuffmanTree(struct used_symbols used[], int count) {
    struct Huffman_tree *nodes[512];
    int n = count;

    
    for (int i = 0; i < count; i++) {
        nodes[i] = createNode(used[i].symbol, used[i].freq, NULL, NULL);
    }

    
    while (n > 1) {
        
        struct Huffman_tree *left = nodes[0];
        struct Huffman_tree *right = nodes[1];

        
        struct Huffman_tree *parent =
            createNode(0, left->freq + right->freq, left, right);

        
        for (int i = 2; i < n; i++) {
            nodes[i - 2] = nodes[i];
        }
        n -= 2;

        
        nodes[n] = parent;
        n++;

        
        for (int i = 0; i < n - 1; i++) {
            for (int j = i + 1; j < n; j++) {
                if (nodes[i]->freq > nodes[j]->freq||(nodes[i]->freq==nodes[j]->freq&&nodes[i]->symbol>nodes[j]->symbol)) {
                    struct Huffman_tree *tmp = nodes[i];
                    nodes[i] = nodes[j];
                    nodes[j] = tmp;
                }
            }
        }
    }
    return nodes[0];
}

void charactercounter(FILE *fptr){
    int c;
    for(int i=0;i<256;i++){
        frequencies[i].freq=0;
        frequencies[i].symbol=(unsigned char)i;
    }
    while((c=fgetc(fptr))!=EOF){
        frequencies[c].freq++;
    }
    rewind(fptr);
}

int compactsortedlist(){
    unsigned int slist[256];
    int k=0;
    for (int i=0;i<256;i++){
        if (frequencies[i].freq>0){
            used[k].symbol=frequencies[i].symbol;
            used[k].freq=frequencies[i].freq;
            k++;
        }
    }
    for(int i=0;i<k-1;i++){
        for(int j=i+1;j<k;j++){
            if(used[i].freq>used[j].freq||(used[i].freq==used[j].freq && used[i].symbol>used[j].symbol)){
                struct used_symbols temp=used[i];
                used[i]=used[j];
                used[j]=temp;
            }
        }
    }
    return k;
}

void traverse_tree(struct Huffman_tree *root,int *path,int depth){

    if(root==NULL){return;}
    if (root->left_branch==NULL&&root->right_branch==NULL){
        int s=root->symbol;
        code_generate[s].symbol=s;
        code_generate[s].length=depth;
        for(int i=0;i<depth;i++){
            code_generate[s].code[i]=path[i];
        }
        printf("Symbol 0x%02X -> Code: ",s);
        for(int i=0;i<depth;i++){
            printf("%d",path[i]);
        }
        printf("\n");
        return;
    }
    path[depth]=0;
    traverse_tree(root->left_branch,path,depth +1);
    path[depth]=1;
    traverse_tree(root->right_branch,path,depth +1);

}

void write_bits(FILE *out, struct BitBuffer *bb, int *code, int length) {
    for (int i = 0; i < length; i++) {
        bb->buffer <<= 1;
        bb->buffer |= code[i]; 
        bb->bits_filled++;

        if (bb->bits_filled == 8) {
            fputc(bb->buffer, out);
            bb->buffer = 0;
            bb->bits_filled = 0;
        }
    }
}

void flush_buffer(FILE *out, struct BitBuffer *bb) {
    if (bb->bits_filled > 0) {
        bb->buffer <<= (8 - bb->bits_filled);
        fputc(bb->buffer, out);
        bb->buffer = 0;
        bb->bits_filled = 0;
    }
}

void compress_file(const char *input_file, const char *output_file) {
    FILE *in = fopen(input_file, "rb");
    FILE *out = fopen(output_file, "wb");

    if (!in || !out) {
        printf("Cannot open files!\n");
        return;
    }
    for(int i=0;i<256;i++){
        fwrite(&frequencies[i].freq,sizeof(unsigned int),1,out);
    }

    struct BitBuffer bb = {0, 0};
    int c;

    while ((c = fgetc(in)) != EOF) {
        write_bits(out, &bb, code_generate[c].code, code_generate[c].length);
    }

    flush_buffer(out, &bb);

    fclose(in);
    fclose(out);
}

struct BitReader {
    FILE *file;
    unsigned char buffer;
    int bits_left;
};

void init_bit_reader(struct BitReader *br, FILE *file) {
    br->file = file;
    br->buffer = 0;
    br->bits_left = 0;
}

int read_bit(struct BitReader *br) {
    if (br->bits_left == 0) {
        int c = fgetc(br->file);
        if (c == EOF) return -1; 
        br->buffer = (unsigned char)c;
        br->bits_left = 8;
    }
    int bit = (br->buffer >> 7) & 1; 
    br->buffer <<= 1;                
    br->bits_left--;
    return bit;
}

void free_tree(struct Huffman_tree *root){
    if(!root)return;
    free_tree(root->left_branch);
    free_tree(root->right_branch);
    free(root);
}

void decompress_file(const char *input_file, const char *output_file) {
    FILE *in = fopen(input_file, "rb");
    FILE *out = fopen(output_file, "wb");

    if (!in || !out) {
        printf("Cannot open files!\n");
        if (in) fclose(in);
        if (out) fclose(out);
        return;
    }

    
    for (int i = 0; i < 256; i++) {
        frequencies[i].symbol = (unsigned char)i;
        fread(&frequencies[i].freq, sizeof(unsigned int), 1, in);
    }

    
    int used_count = compactsortedlist();  
    unsigned long total_symbols = 0;
    for (int i = 0; i < used_count; i++) {
        total_symbols += used[i].freq;
    }

    if (used_count == 0) {
        printf("Empty or corrupted file.\n");
        fclose(in);
        fclose(out);
        return;
    }

    
    struct Huffman_tree *root = buildHuffmanTree(used, used_count);

    
    struct BitReader br;
    init_bit_reader(&br, in);

    struct Huffman_tree *current = root;
    unsigned long symbols_written = 0;

    
    while (symbols_written < total_symbols) {
        int bit = read_bit(&br);
        if (bit == -1) {
            printf("Unexpected EOF while reading compressed data!\n");
            break;
        }

        if (bit == 0)
            current = current->left_branch;
        else
            current = current->right_branch;

        
        if (current->left_branch == NULL && current->right_branch == NULL) {
            fputc(current->symbol, out);
            symbols_written++;
            current = root; 
        }
    }

    fclose(in);
    fclose(out);
    free_tree(root);

    printf("Decompression done! Output file: %s\n", output_file);
}

int main() {
    char filename[256], outputfilename[512];
    int choice;

    printf("Huffman File Compressor/Decompressor\n");
    printf("1. Compress a file\n");
    printf("2. Decompress a file\n");
    printf("Enter your choice (1 or 2): ");
    scanf("%d", &choice);
    getchar();

    printf("Enter the filename: ");
    fgets(filename, sizeof(filename), stdin);
    filename[strcspn(filename, "\n")] = '\0';

    if (choice == 1) {
        
        FILE *fptr = fopen(filename, "rb");
        if (!fptr) {
            printf("Cannot open %s\n", filename);
            return 1;
        }

        charactercounter(fptr);
        int count = compactsortedlist();
        struct Huffman_tree *root = buildHuffmanTree(used, count);

        int path[256];
        traverse_tree(root, path, 0);

        snprintf(outputfilename, sizeof(outputfilename), "%s.huff", filename);
        compress_file(filename, outputfilename);

        fclose(fptr);
        printf("Compression done! Output file: %s\n", outputfilename);
        free_tree(root);
    } else if (choice == 2) {

        snprintf(outputfilename, sizeof(outputfilename), "%s_dec.txt", filename);
        decompress_file(filename, outputfilename);
    } else {
        printf("Invalid choice!\n");
    }
    return 0;
}
