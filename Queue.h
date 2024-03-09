class Queue {

  public:

    Queue();

    void push(float value);

    float getAverage();

    void output();

    int length;

  private:

    float data[72];
    
    int index;
};