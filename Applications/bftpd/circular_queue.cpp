#include <cstdio>

#include <memory>
#include <mutex>

/**
* Important Usage Note: This library reserves one spare entry for queue-full detection
* Otherwise, corner cases and detecting difference between full/empty is hard.
* You are not seeing an accidental off-by-one.
*/

template <class T>
class circular_buffer {
public:
	circular_buffer(size_t size) :
		buf_(std::unique_ptr<T[]>(new T[size])),
		size_(size)
	{
		//empty constructor
	}



	void put(T item)
	{
		std::lock_guard<std::mutex> lock(mutex_);

		buf_[head_] = item;
		head_ = (head_ + 1) % size_;

		if(head_ == tail_)
		{
			tail_ = (tail_ + 1) % size_;
		}
	}

	int insert(char *item, int len, std::ofstream &log)
	{
		//return the number of items inserted in the buffer 
		for(int i=0; i<len; i++)
		{
			if(!full())
				put(item[i]);
			else 
				return i;
		}

		return len;
	}

	int insert(char *item, int len)
	{
		//return the number of items inserted in the buffer 
		for(int i=0; i<len; i++)
		{
			if(!full())
				put(item[i]);
			else 
				return i;
		}

		return len;
	}

	int remove(char*chunk , int len)
	{
		int i = 0;
		for(i=0; i<len; i++)
		{
			if(!empty())
				chunk[i]=get();
		}
		if(i!=len)
			printf("There is some error here");
		chunk[len] = '\0';
		return len;
	}

	T get(void)
	{
		std::lock_guard<std::mutex> lock(mutex_);

		if(empty())
		{
			return T();
		}

		//Read data and advance the tail (we now have a free space)
		auto val = buf_[tail_];
		tail_ = (tail_ + 1) % size_;

		return val;
	}

	void reset(void)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		head_ = tail_;
	}

	bool empty(void)
	{
		//if head and tail are equal, we are empty
		return head_ == tail_;
	}

	bool full(void)
	{
		//If tail is ahead the head by 1, we are full
		return ((head_ + 1) % size_) == tail_;
	}

	size_t size(void)
	{
		return size_ - 1;
	}

private:
	std::mutex mutex_;
	std::unique_ptr<T[]> buf_;
	size_t head_ = 0;
	size_t tail_ = 0;
	size_t size_;
};

int main(void)
{
	circular_buffer<char> circle(10);
	printf("\n === CPP Circular buffer check ===\n");
	printf("Capacity: %zu\n", circle.size());
	uint32_t x = '1';
	printf("Put 1, val: %c\n", x);
	circle.put(x);
	x = circle.get();
	printf("Popped: %c\n", x);
	printf("Empty: %d\n", circle.empty());

	printf("Adding 9 values\n");
	char * test;
	test = new char[11];
	for(uint32_t i = 0; i<9; i++)
		test[i] = 65+i;
	test[11] = '\0';

	printf("number of items inserted: %d\n",circle.insert(test,11));

	printf("Full: %d\n", circle.full());

	//printf("Reading back values: ");
	
	// for(uint32_t i=0; i<9; i++)
	// 	test[i]='p';

	// circle.remove(test,3);

	// for(uint32_t i=0; i<3; i++)
	// 	printf("%c ", test[i]);
	// 	printf(" ");
	// printf("\n");

	printf("Reading back values: ");
	while(!circle.empty())
	{
		printf("%c ", circle.get());
	}

	circle.insert(test+3,7);
	circle.insert(test,3);

	printf("Reading back values: ");
	while(!circle.empty())
	{
		printf("%c ", circle.get());
	}

	return 0;
}
