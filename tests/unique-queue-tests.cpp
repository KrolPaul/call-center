#include <gtest/gtest.h>
#include "../include/unique-queue.h"
#include "../include/cdr.h"

using namespace cdr;

class UniqueQueueTest : public ::testing::Test{
protected:
	void SetUp(){
        queue.setRejectRepeated(true);
        entity1.id = "1";
        entity2.id = "2";
        entity3.id = "3";
        queue.setMaxSize(2);
	}
	void TearDown(){
	}
    struct Type{
        std::string id;
        int data;
        std::string getId() const {return id;}
    };
    UniqueQueue<Type> queue;
    Type entity1;
    Type entity2;
    Type entity3;
};


TEST_F(UniqueQueueTest, pushNewUniqueElement){
    EXPECT_EQ(queue.push(entity1), UniqueQueue<Type>::EC::inserted);
    ASSERT_EQ(queue.getSize(), 1);
}

TEST_F(UniqueQueueTest, pushNotUniqueElementReject){
    entity1.data = 7;
    queue.push(entity1);

    entity1.data = 8;
    EXPECT_EQ(queue.push(entity1), UniqueQueue<Type>::EC::alreadyInQueue);
    ASSERT_EQ(queue.getSize(), 1);
    ASSERT_EQ(queue.top().data, 7);
}

TEST_F(UniqueQueueTest, pushNotUniqueElementNoReject){
    queue.setRejectRepeated(false);
    entity1.data = 7;
    queue.push(entity1);

    entity1.data = 8;
    EXPECT_EQ(queue.push(entity1), UniqueQueue<Type>::EC::reassigned);
    ASSERT_EQ(queue.getSize(), 1);
    ASSERT_EQ(queue.top().data, 8);
}

TEST_F(UniqueQueueTest, pushWhenOverloaded){
    queue.push(entity1);
    queue.push(entity2);

    EXPECT_EQ(queue.push(entity3), UniqueQueue<Type>::EC::overload);
    ASSERT_EQ(queue.getSize(), 2);
}

TEST_F(UniqueQueueTest, tryPopFromEmptyQueue){
    ASSERT_FALSE(queue.tryPop(entity1));
}

TEST_F(UniqueQueueTest, tryPopFromNotEmptyQueue){
    queue.push(entity1);
    queue.push(entity2);

    ASSERT_TRUE(queue.tryPop(entity3));
    ASSERT_EQ(queue.getSize(), 1);
}

TEST_F(UniqueQueueTest, checkExistingElementInQueue){
    queue.push(entity1);
    ASSERT_TRUE(queue.isInQueue(entity1.getId()));
}

TEST_F(UniqueQueueTest, checkNotExistingElementInQueue){
    ASSERT_FALSE(queue.isInQueue(entity1.getId()));
}

TEST_F(UniqueQueueTest, emptyQueueIsEmpty){
    ASSERT_TRUE(queue.isEmpty());
}

TEST_F(UniqueQueueTest, notEmptyQueueIsNotEmpty){
    queue.push(entity1);
    ASSERT_FALSE(queue.isEmpty());
}

TEST_F(UniqueQueueTest, checkSize){
    queue.push(entity1);
    queue.push(entity2);

    ASSERT_EQ(queue.getSize(), 2);
}

TEST_F(UniqueQueueTest, isInQueue){
    queue.push(entity1);
    
    ASSERT_TRUE(queue.isInQueue(entity1.id));
    ASSERT_FALSE(queue.isInQueue(entity2.id));
}

TEST_F(UniqueQueueTest, eraseExistedElement){
    queue.push(entity1);
    queue.push(entity2);

    ASSERT_TRUE(queue.erase(entity1.id));
    EXPECT_EQ(queue.getSize(), 1);
    ASSERT_FALSE(queue.isInQueue(entity1.id));
}

TEST_F(UniqueQueueTest, eraseNotExistedElement){
    queue.push(entity1);
    queue.push(entity3);
    
    ASSERT_FALSE(queue.erase(entity2.id));
    ASSERT_EQ(queue.getSize(), 2);
}


TEST(cdr, cdrIdEqToPhoneNumber){
    Cdr cdr;
    cdr.phoneNumber = "12345";

    ASSERT_EQ(cdr.phoneNumber, cdr.getId());
}