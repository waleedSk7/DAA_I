# Graph Algorithms and Analysis Project

This repository contains various implementations of graph algorithms along with performance analyses on different datasets. It also includes a web interface that displays experimental results and additional insights.

## Important Note About Large Dataset Files

**Some dataset files exceed GitHub's file size limit of 100MB and are not included directly in this repository:**
- `Cleaned Dataset/as-skitter-clean.txt` (142MB)
- `Uncleaned Dataset/as-skitter.txt` (142MB)
- `as-skitter-clean.txt` (142MB)

These files can be downloaded from the Google Drive link provided in the "Webpage and cleaned dataset" section below.

## Data Setup

1. **Download Datasets:**  
     
   - [Email-Enron Dataset](https://snap.stanford.edu/data/email-Enron.html)  
   - [AS-Skitter Dataset](https://snap.stanford.edu/data/as-Skitter.html)  
   - [Wiki-Vote Dataset](https://snap.stanford.edu/data/wiki-Vote.html)

   

2. **Prepare the Data:**  
     
   - Unzip the downloaded archives.  
   - Ensure each dataset file is cleaned (e.g., remove any header comments, and ensure the first line displays the number of nodes and edges)

   

3. **Organize Files:**  
     
   - Rename the files as necessary:  
     - Rename the Enron dataset file to `enron.txt`  
     - Rename the Wiki-Vote dataset file to `wiki.txt`  
     - Rename the AS-Skitter dataset file to `skitter.txt`

## Building and Running the Algorithms

### For the Algorithm Implementations Compilation

Format

g++ \-O3 \<filename\>.cpp \-o \<output\_filename\>.out

* Compile `els.cpp`:  
    
  g++ \-O3 els \-o els.cpp  
    
  ./els.exe   
    
* Compile `tomita.cpp`:  
    
  g++ \-O3 tomita \-o tomita.cpp  
    
  ./tomita.exe  
    
* Compile `chiba.cpp`:  
    
  g++ \-O3 chiba \-o chiba.cpp  
    
  ./chiba.exe

### Sample Program Input

After you run the program it will ask for the relative file path of the input file.
Make sure to give the correct path and that the first line contains the Number of Vertices and Number of Edges which are separated by a space.

### Webpage and cleaned dataset link

Cleaned Dataset Link :- [DAA](https://drive.google.com/drive/folders/1ryaa1RFrjOlgR2xHpGYmLwlYdg9C4WwU?usp=sharing)  
Website Link :- [DAA_I Website](https://waleedSk7.github.io/DAA_I/)

### Team Members

* Sthitaprajna   
  * Contributed to the implementation of the CLIQUES algorithm based on the Tomita et al. (2006) paper  
  * Implemented Arboricity based algorithm based on the Chiba et al. (1985) paper


* Kushagra Mishra  
  * Contributed to the implementation of the CLIQUES algorithm based on the Tomita et al. (2006) paper  
  * Contributed to the implementation of the Bron-Kerbosch algorithm based on the Eppstein et al. (2006) paper


* Riya Agrawal  
  * Implemented Arboricity based algorithm based on the Chiba et al. (1985) paper  
  * Helped develop and host the website 


* Waleed Iqbal Shaikh  
  * Contributed to the implementation of the Bron-Kerbosch algorithm based on the Eppstein et al. (2006) paper  
  * Helped develop and host the website 


* Dhruv Choudhary  
  * Contributed in implementing the arboricity based algorithm based on the Chiba et al. (1985) paper  
  * Authored the report

