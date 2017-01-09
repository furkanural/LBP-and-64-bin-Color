//
//  main.c
//  homework_3
//
//  Created by Furkan Ural on 24/12/2016.
//  Copyright © 2016 Furkan Ural. All rights reserved.
//

#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <opencv2/core/core_c.h>
#include <opencv2/imgcodecs/imgcodecs_c.h>
#include <opencv2/highgui/highgui_c.h>

//qsort algoritmasi icin karsılastirma metodu
int cmp (const void * a, const void * b)
{
    double* arr1 = (double*)a;
    double* arr2 = (double*)b;
    if (arr1[1] > arr2[1])
        return 1;
    else if (arr1[1] < arr2[1])
        return -1;
    else
        return 0;
}

/// dosya yolu belirtilen fotografi okuyan ve geri donen kod blogu
IplImage * read_image(char * path, int iscolor){
    return cvLoadImage(path, iscolor);
}

/// parametre olarak gelen dosyayi belirtilen dizine yazan ve islem sonucunu donen kod blogu
int write_image(char * path, IplImage * image){
    return cvSaveImage(path, image, 0);
}

//görüntünün istenilen pikseline istenilen değeri yazan metod
void quantize_image(IplImage *img_quant, int val, int i, int j){
    CvScalar n;
    n = cvGet2D(img_quant, i,j );
    n.val[0] = val*2;
    cvSet2D(img_quant, i, j, n);
}

//FILE isaretciyle acilmis dosyaya values degerlerini tek satir
//halinda aralarina virgul koyarak yazan metod
void write_file(FILE *f, int image_number, int values[], int size_of_values) {
    fprintf(f, "%d -> ",image_number);
    for (int i=0; i<size_of_values; i++) {
        fprintf(f, "%d, ",values[i]);
    }
    fprintf(f, "\n");
}

//64bin color histogramini hesaplayip geriye yeni olusturulmus goruntuyu
//donen metod. ayrica values dizisinde histogram bilgilerini saklar
IplImage *reduce_to_64_color_with_result(int values[], IplImage *img){
    IplImage *img_quant = cvCreateImage(cvSize(img->width, img->height), IPL_DEPTH_8U, 1);
    
    for (int i = 0; i < img->height; i++) {
        for (int j = 0; j < img->width; j++) {
            CvScalar s;
            s = cvGet2D(img, i, j);
            //yeni RGB degerini hesaplamak icin degerin AND ve dondurulme islemi
            uchar C1 = ((uchar)s.val[0] & 0b11000000 )>>6;
            uchar C2 = ((uchar)s.val[1] & 0b11000000 )>>4;
            uchar C3 = ((uchar)s.val[2] & 0b11000000 )>>2;
            
            //yeni deger icin R, G ve B degerlerinin OR islemi ile birlestirilmesi
            uchar result = C3 | C2 | C1;
            values[(int)result]++;
            
            quantize_image(img_quant, result, i, j);
        }
    }
    return img_quant;
}

//64bin color histogrami hesaplayan metodu kullanarak uretilen goruntuyu parametreye gore
//istenilen konuma kayededen metod
void reduce_to_64_color(IplImage *img, int image_number, FILE *f, bool save_quantize_image, char* path_){
    
    int quant[64] = {0};
    IplImage *img_quant = reduce_to_64_color_with_result(quant, img);
    
    if (save_quantize_image) {
        char path[60];
        sprintf(path, "%s/64bin_%d.jpg",path_, image_number);
        write_image(path, img_quant);
    }
    write_file(f, image_number, quant, 64);
}

//path konumunda bulunan txt dosyasini okuyarak uniform degerleri
// values dizisinda saklayan metod
void read_uniform_vals(int values[], char *path){
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        printf("read_uniform_vals(..) -> look up table not found\n");
        exit(1);
    }
    char buf[8];
    int index = 0;
    int val = 0;
    while (fscanf(f, " %s\n", buf) == 1) { //okunacak satir oldugunca
        sscanf (buf,"%d,%d", &val, &index);
        values[index] = val;
    }
    fclose(f);
}

//val degerine karsilik gelen uniform degeri bulan ve indeks bilgisini
// geri donen metod
int find_uniform_LBP_index(int uniform_vals[], int val){
    for (int i=0; i < 58; i++) {
        if (uniform_vals[i] == val){
            return i;
        }
    }
    return 58;
}

//LBP histogramini hesaplayan ve histograma gore uretilen fotografi geriye donen metod
IplImage *reduce_to_LBP_with_result(int values[], int uniform_vals[], IplImage *img) {
    IplImage *img_quant = cvCreateImage(cvSize(img->width, img->height), IPL_DEPTH_8U, 1);
    
    for (int i = 1; i < (img->height - 1); i++) {
        for (int j = 1; j < (img->width - 1); j++) {
            CvScalar s;
            s = cvGet2D(img, i, j);
            int center = 0;
            //8 komsulugunda bulunan pikseller ile karsilastirma
            if (cvGet2D(img, i, j-1).val[0] > s.val[0]) {
                center += 1;
            }
            if (cvGet2D(img, i+1, j-1).val[0] > s.val[0]) {
                center += 2;
            }
            if (cvGet2D(img, i+1, j).val[0] > s.val[0]) {
                center += 4;
            }
            if (cvGet2D(img, i+1, j+1).val[0] > s.val[0]) {
                center += 8;
            }
            if (cvGet2D(img, i, j+1).val[0] > s.val[0]) {
                center += 16;
            }
            if (cvGet2D(img, i-1, j+1).val[0] > s.val[0]) {
                center += 32;
            }
            if (cvGet2D(img, i-1, j).val[0] > s.val[0]) {
                center += 64;
            }
            if (cvGet2D(img, i-1, j-1).val[0] > s.val[0]) {
                center += 128;
            }
            
            //bulunan center degerinin uniform karsiligi
            center = find_uniform_LBP_index(uniform_vals, center);
            values[center]++;
            quantize_image(img_quant, center, i, j);
        }
    }
    return img_quant;
}

//LBP histogramini hesaplayan metodu kullanarak olusan fotografi isteilen konuma kayeden metod
void reduce_to_LBP(IplImage *img, int uniform_vals[], int image_number, FILE *f, bool save_quantize_image, char* path_){
    int result[59] = {0};
    IplImage *quantize_img = reduce_to_LBP_with_result(result, uniform_vals, img);
    
    if (save_quantize_image) {
        char path[60];
        sprintf(path, "%s/lbp_%d.jpg",path_, image_number);
        write_image(path, quantize_img);
    }
    
    write_file(f, image_number, result, 59);
}

//uniform degerlerin hesaplanmasi sirasinda 8bitlik sayi degerinin
//saga dondurulmesine yardim eden metod
uint8_t rotr8 (uint8_t value, unsigned int count) {
    const unsigned int mask = (CHAR_BIT*sizeof(value)-1);
    count &= mask;
    return (value>>count) | (value<<( (-count) & mask ));
}

//LBP algoritmasi icin look up table uretilmesinde yardimci olan metod
int write_on_table_lbp(FILE *f,uint8_t val, int size, int index ){
    for (int i= 0; i < 8; i++) {
        fprintf(f, "%0*d,%0*d\n", size, val,size, index++);
        val = rotr8(val, 1);
    }
    return index;
}

//LBP algoritmasinda kullanilacak uniform degerleri hesaplayan metod
void create_look_up_table_for_lbp(char *path) {
    FILE *f = fopen(path, "w");
    if (f == NULL) {
        printf("create_look_up_table_for_lbp(.) -> look up table can not created");
        exit(1);
    }
    int size=3;
    int index = 0;
    fprintf(f, "%0*d,%0*d\n", size, 0b00000000,size, index++);
    //1-0 ve 0-1 gecisleri kurala uyan degerleri dondur ve yaz
    index = write_on_table_lbp(f,0b10000000, size, index);
    index = write_on_table_lbp(f,0b11000000, size, index);
    index = write_on_table_lbp(f,0b11100000, size, index);
    index = write_on_table_lbp(f,0b11110000, size, index);
    index = write_on_table_lbp(f,0b11111000, size, index);
    index = write_on_table_lbp(f,0b11111100, size, index);
    index = write_on_table_lbp(f,0b11111110, size, index);
    fprintf(f, "%0*d,%0*d\n", size, 0b11111111,size, index);
    fclose(f);
}

//iki goruntu arasinda oklit mesafesinin hesaplanmasi
double euclidean_distince(int values[], int train_values[], int size){
    double sum=0;
    for (int i=0; i < size; i++) {
        sum += pow((double)(values[i] - train_values[i]), 2);
    }
    return sqrt(sum);
}

//bir goruntu ile egitim verisi arasinda ki tum goruntulerin oklit mesafesinin hesaplanmasi
void distince(FILE *f, int values[], double distinces[][2], int train_size) {
    char buf[10];
    int val=0;
    int train_values[train_size];
    
    int image_num;
    
    fscanf(f, "%s ->", buf);
    sscanf(buf, "%d ->", &image_num);
    
    for (int i=0; i < train_size; i++) {
        fscanf(f, " %s,", buf);
        sscanf(buf, "%d,", &val);
        train_values[i] = val;
    }
    distinces[image_num - 1][0] = image_num;
    distinces[image_num - 1][1] = euclidean_distince(train_values,values, train_size);
}

//oklit mesafesinin hesaplanip sonuclarinin path dizinine yazilmasi
void find_distinces(char *path, int file_count, double distinces[][2], int values[], int train_size){
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        printf("find_distinces(.....) -> distinces file not found");
        exit(1);
    }
    
    for (int i=0; i < file_count; i++) {
        distince(f, values, distinces, train_size);
    }
    
    //mesafelerin kucukten buyuge siralanamsi
    qsort(distinces, file_count, 2*sizeof(double), cmp);
    
    fclose(f);
    
}

//64 color histogram algoritmasini belirtilen klasor altinda ki fotograflar ile
//sistemin egitilmesi. sonuclar txt dosyasinda tutulur
void train_64_color(int file_count, char *path_, bool save_quantize_image) {
    char path[60];
    sprintf(path, "%s/64_bin_color_result.txt", path_);
    FILE *f = fopen(path, "w");
    if (f == NULL) {
        printf("train_64_color(...) -> file can not created");
        exit(1);
    }
    
    for (int i=file_count; i>0; i--) {
        char path[60];
        sprintf(path, "%s/Color/%d.jpg",path_, i);
        IplImage *img = read_image(path, CV_LOAD_IMAGE_COLOR);
        reduce_to_64_color(img, i, f, save_quantize_image, path_);
    }
    fclose(f);
}

//LBP histogram algoritmasini belirtilen klasor altinda ki fotograflar ile
//sistemin egitilmesi. sonuclar txt dosyasinda tutulur
void train_LBP(int file_count, char *path_, bool look_up_table, bool save_quantize_image) {
    char path[60];
    char look_up_path[60];
    sprintf(path, "%s/LBP_result.txt", path_);
    sprintf(look_up_path, "%s/look_up_table.txt", path_);
    
    if (look_up_table) {
        create_look_up_table_for_lbp(look_up_path);
    }
    
    int uniform_values[58];
    read_uniform_vals(uniform_values, look_up_path);
    
    FILE *f = fopen(path, "w");
    if (f == NULL) {
        printf("train_LBP(....) -> file can not created");
        exit(1);
    }
    
    for (int i=file_count; i>0; i--) {
        char path[60];
        sprintf(path, "%s/Texture/%d.jpg", path_, i);
        IplImage *img = read_image(path, CV_LOAD_IMAGE_GRAYSCALE);
        reduce_to_LBP(img, uniform_values, i, f, save_quantize_image, path_);
    }
    fclose(f);
}

//egitim verileri uzerinde test islemi
void test_64_color(char *path_, char *output_folder, IplImage *img, int file_count){
    int values[64] = {0};
    char path[60];
    
    double distinces[file_count][2];
    sprintf(path, "%s/64_bin_color_result.txt", path_);
    reduce_to_64_color_with_result(values, img);
    find_distinces(path, file_count, distinces, values, 64);
    
    for (int i=0; i < file_count; i++) {
        char path[60];
        sprintf(path, "%s/Color/%d.jpg", path_, (int)distinces[i][0]);
        IplImage *img_result = read_image(path, CV_LOAD_IMAGE_COLOR);
        sprintf(path, "%s/%d.jpg", output_folder, i);
        write_image(path, img_result);
    }
}

//egitim verileri uzerinde test islemi
void test_LBP(char *path_, char *output_folder, IplImage *img, int file_count){
    int values[59] = {0};
    int uniform_values[58];
    char look_up_path[60];
    char path[60];

    double distinces[file_count][2];
    
    sprintf(look_up_path, "%s/look_up_table.txt", path_);
    sprintf(path, "%s/LBP_result.txt", path_);
    IplImage *grayscale = cvCreateImage(cvSize(img->width, img->height), IPL_DEPTH_8U, 1);
    cvConvertImage(img, grayscale, CV_BGR2GRAY);
    
    
    read_uniform_vals(uniform_values, look_up_path);
    reduce_to_LBP_with_result(values, uniform_values, grayscale);
    find_distinces(path, file_count, distinces, values, 59);
    
    for (int i=0; i < file_count; i++) {
        char path[60];
        sprintf(path, "%s/Texture/%d.jpg", path_, (int)distinces[i][0]);
        IplImage *img_result = read_image(path, CV_LOAD_IMAGE_COLOR);
        sprintf(path, "%s/%d.jpg", output_folder, i);
        write_image(path, img_result);
    }
    
}


int main(int argc, const char * argv[]) {
    char *path_prefix = "Dataset";
    
    train_64_color(50, path_prefix, true);
    train_LBP(50, path_prefix, true, true);
    
    IplImage *lbp_1 = read_image("Dataset/Texture/1.jpg", CV_LOAD_IMAGE_COLOR);
    IplImage *lbp_2 = read_image("Dataset/Texture/30.jpg", CV_LOAD_IMAGE_COLOR);
    IplImage *lbp_3 = read_image("Dataset/Texture/48.jpg", CV_LOAD_IMAGE_COLOR);
    
    IplImage *color_64bin_1 = read_image("Dataset/Color/1.jpg", CV_LOAD_IMAGE_COLOR);
    IplImage *color_64bin_2 = read_image("Dataset/Color/31.jpg", CV_LOAD_IMAGE_COLOR);
    IplImage *color_64bin_3 = read_image("Dataset/Color/32.jpg", CV_LOAD_IMAGE_COLOR);
    
    char *out_lbp_1 = "texture_1";
    char *out_lbp_2 = "texture_30";
    char *out_lbp_3 = "texture_48";
    
    char *out_64bin_1 = "color_1";
    char *out_64bin_2 = "color_31";
    char *out_64bin_3 = "Dataset/output_color_3";
    
    
    test_LBP(path_prefix, out_lbp_1, lbp_1, 50);
    test_LBP(path_prefix, out_lbp_2, lbp_2, 50);
    test_LBP(path_prefix, out_lbp_3, lbp_3, 50);
    
    test_64_color(path_prefix, out_64bin_1, color_64bin_1, 50);
    test_64_color(path_prefix, out_64bin_2, color_64bin_2, 50);
    test_64_color(path_prefix, out_64bin_3, color_64bin_3, 50);
    
    
    return 0;
}
