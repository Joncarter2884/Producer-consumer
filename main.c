

void producer(const char source_file[], int bufferSize, int chunkSize, int fd[2]) {
    FILE *filePointer = fopen(source_file, "rt");
    const char *name = "OS"; /* name of the shared memory object */
    int shmFd;              /* shared memory file descriptor */
    void *ptr, *tempPtr;    /* pointer to shared memory object */
    char message[chunkSize];
    size_t len = chunkSize - 1, bytes_wrote = 0;
    int shMemPrdCharCount = 0;
    char *outputBuffer = malloc(sizeof(char) * (bufferSize + 100));
    sem_t *myWriteSem, *myReadSem;

    /* create the shared memory object */
    shmFd = shm_open(name, O_CREAT | O_RDWR, 0666);
    /* configure the size of the shared memory object */
    ftruncate(shmFd, bufferSize);

    /* memory map the shared memory object */
    ptr = tempPtr = mmap(0, bufferSize, PROT_WRITE, MAP_SHARED, shmFd, 0);

    myWriteSem = sem_open(MY_WRITE_SEM, O_CREAT, S_IRWXU, 1);
    if (myWriteSem == NULL) {
        perror("In sem_open() opening write semaphore.");
        exit(1);
    }

    myReadSem = sem_open(MY_READ_SEM, O_CREAT, S_IRWXU, 0);
    if (myReadSem == NULL) {
        perror("In sem_open() opening read semaphore.");
        exit(1);
    }

    /* write to the shared memory object */
    while (len == chunkSize - 1) {
        sem_wait(myWriteSem);
        len = fread(message, 1, chunkSize - 1, filePointer);
        message[len] = '\0';
        len = sprintf(tempPtr, "%s", message);

        sprintf(outputBuffer,"PARENT: IN = %lu\n", len);
        write(outFile, outputBuffer, strlen(outputBuffer));
        sprintf(outputBuffer,"PARENT: ITEM = %s\n", message);
        write(outFile, outputBuffer, strlen(outputBuffer));
        bytes_wrote += strlen(message);
        shMemPrdCharCount += strlen(message);
        tempPtr = ptr + ((((bytes_wrote % bufferSize) + chunkSize) < bufferSize)
                ? (bytes_wrote % bufferSize)
                : 0);
        if (tempPtr == ptr)
            sem_post(myReadSem);
    }
    sprintf(outputBuffer,"Producer Done Producing\n");
    write(outFile, outputBuffer, strlen(outputBuffer));
    fclose(filePointer);
    char buffer[20];
    sprintf(buffer, "%d", shMemPrdCharCount);
    close(fd[0]);
    write(fd[1], buffer, sizeof buffer);
    sprintf(outputBuffer,"PARENT: The parent value of shMemPrdCharCount = %d\n", shMemPrdCharCount);
    write(outFile, outputBuffer, strlen(outputBuffer));
    free(outputBuffer);
}



void consumer(const char target_file[], int bufferSize, int chunkSize, int fd[2]) {
    FILE *filePointer;
    const char *name = "OS"; 
    int shmFd;            
    void *ptr, *tempPtr;  
    char *message;
    size_t len = chunkSize - 1, bytes_wrote = 0;
    int shMemCsrCharCount = 0;
    int shMemPrdCharCount;
    sem_t *myWriteSem, *myReadSem;
    char *outputBuffer = malloc(sizeof(char) * (bufferSize + 100));

    filePointer = fopen(target_file, "w");

    shmFd = shm_open(name, O_RDONLY, 0666); /* open the shared memory object */

    /* memory map the shared memory object */
    ptr = tempPtr = mmap(0, bufferSize, PROT_READ, MAP_SHARED, shmFd, 0);

    // sleep(1); // sleeping for 1 sec in order to allow producer to take a lead
    // over consumer

    myWriteSem = sem_open(MY_WRITE_SEM, O_CREAT, S_IRWXU, 1);
    if (myWriteSem == NULL) {
        perror("In sem_open() opening write semaphore.");
        exit(1);
    }

    myReadSem = sem_open(MY_READ_SEM, O_CREAT, S_IRWXU, 0);
    if (myReadSem == NULL) {
        perror("In sem_open() opening read semaphore.");
        exit(1);
    }


    /* read from the shared memory object */
    while (len == chunkSize - 1) {
        sem_wait(myReadSem);
        message = (char *)tempPtr;
        len = strlen(message);
        fprintf(filePointer, "%s", message);

        sprintf(outputBuffer ,"CHILD: IN = %lu\n", len);
        write(outFile, outputBuffer, strlen(outputBuffer));
        sprintf(outputBuffer ,"CHILD: ITEM = %s\n", message);
        write(outFile, outputBuffer, strlen(outputBuffer));

        bytes_wrote += len;
        shMemCsrCharCount += len;
        tempPtr = ptr + ((((bytes_wrote % bufferSize) + chunkSize) < bufferSize)
                ? (bytes_wrote % bufferSize)
                : 0);
        if (tempPtr == ptr)
            sem_post(myWriteSem);
    }

    /*printf("Consumer Done Consuming\n");*/
    sprintf(outputBuffer, "Consumer Done Consuming\n");
    write(outFile, outputBuffer, strlen(outputBuffer));

    /* remove the shared memory object */
    shm_unlink(name);
    fclose(filePointer);
    char buffer[20];
    close(fd[1]);
    read(fd[0], buffer, sizeof buffer);
    sscanf(buffer,"%d", &shMemPrdCharCount);
    sprintf(outputBuffer, "CHILD: The parent value of shMemPrdCharCount = %d\n"
            "CHILD: The child value of shMemCsrCharCount = %d\n",
            shMemPrdCharCount, shMemCsrCharCount
           );
    write(outFile, outputBuffer, strlen(outputBuffer));
    free(outputBuffer);
}
