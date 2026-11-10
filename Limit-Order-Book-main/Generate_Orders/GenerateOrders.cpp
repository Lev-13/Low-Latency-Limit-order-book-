#include "GenerateOrders.hpp"
#include "../Limit_Order_Book/Book.hpp"
#include "../Limit_Order_Book/Limit.hpp"
#include "../Limit_Order_Book/Order.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <random>
#include <algorithm>
#include <functional>
#include <numeric>

GenerateOrders::GenerateOrders(Book* _book) : book(_book), gen(rd()) {}

void GenerateOrders::market()
{
    std::uniform_int_distribution<> sharesDist(1, 1000);
    std::uniform_int_distribution<> buyOrSellDist(0, 1);

    int shares = sharesDist(gen);
    bool buyOrSell = buyOrSellDist(gen);

    file << "Market " << orderId << " " << buyOrSell << " " << shares << "\n";
    book->marketOrder(orderId, buyOrSell, shares);
    orderId++;
}

void GenerateOrders::addLimit()
{
    std::uniform_int_distribution<> sharesDist(1, 1000);
    std::uniform_int_distribution<> buyOrSellDist(0, 1);

    int centerPrice = 300;
    if (book->getHighestBuy() != nullptr && book->getLowestSell() != nullptr) {
        centerPrice = (book->getHighestBuy()->getLimitPrice() + book->getLowestSell()->getLimitPrice()) / 2;
    } else if (book->getHighestBuy() != nullptr) {
        centerPrice = book->getHighestBuy()->getLimitPrice();
    } else if (book->getLowestSell() != nullptr) {
        centerPrice = book->getLowestSell()->getLimitPrice();
    }
    std::normal_distribution<> limitPriceDist(centerPrice, 25);

    int shares = sharesDist(gen);
    bool buyOrSell = buyOrSellDist(gen);
    int limitPrice = static_cast<int>(limitPriceDist(gen));

    if (buyOrSell)
    {
        if (book->getLowestSell() != nullptr && limitPrice >= book->getLowestSell()->getLimitPrice()) {
            limitPrice = book->getLowestSell()->getLimitPrice() - 1;
        }
    } else {
        if (book->getHighestBuy() != nullptr && limitPrice <= book->getHighestBuy()->getLimitPrice()) {
            limitPrice = book->getHighestBuy()->getLimitPrice() + 1;
        }
    }
    if (limitPrice <= 0) limitPrice = 1;

    file << "AddLimit " << orderId << " " << buyOrSell << " " << shares << " " << limitPrice << "\n";
    book->addLimitOrder(orderId, buyOrSell, shares, limitPrice);
    activeLimitOrders.push_back(orderId);
    orderId++;
}

void GenerateOrders::cancelLimit()
{
    if (activeLimitOrders.size() <= 10000)
    {
        addLimit();
        return;
    }
    std::uniform_int_distribution<> dist(0, activeLimitOrders.size() - 1);
    int idx = dist(gen);
    int targetId = activeLimitOrders[idx];
    activeLimitOrders[idx] = activeLimitOrders.back();
    activeLimitOrders.pop_back();

    Order* order = book->searchOrderMap(targetId);
    if (order == nullptr)
    {
        addLimit();
        return;
    }

    file << "CancelLimit " << targetId << "\n";
    book->cancelLimitOrder(targetId);
}

void GenerateOrders::modifyLimit()
{
    if (activeLimitOrders.empty())
    {
        addLimit();
        return;
    }

    int centerPrice = 300;
    if (book->getHighestBuy() != nullptr && book->getLowestSell() != nullptr) {
        centerPrice = (book->getHighestBuy()->getLimitPrice() + book->getLowestSell()->getLimitPrice()) / 2;
    }
    std::uniform_int_distribution<> sharesDist(1, 1000);
    std::normal_distribution<> limitPriceDist(centerPrice, 25);

    int shares = sharesDist(gen);

    std::uniform_int_distribution<> dist(0, activeLimitOrders.size() - 1);
    int idx = dist(gen);
    int targetId = activeLimitOrders[idx];

    Order* order = book->searchOrderMap(targetId);
    if (order == nullptr)
    {
        activeLimitOrders[idx] = activeLimitOrders.back();
        activeLimitOrders.pop_back();
        addLimit();
        return;
    }

    bool buyOrSell = order->getBuyOrSell();
    int limitPrice = static_cast<int>(limitPriceDist(gen));

    if (buyOrSell)
    {
        if (book->getLowestSell() != nullptr && limitPrice >= book->getLowestSell()->getLimitPrice()) {
            limitPrice = book->getLowestSell()->getLimitPrice() - 1;
        }
    } else {
        if (book->getHighestBuy() != nullptr && limitPrice <= book->getHighestBuy()->getLimitPrice()) {
            limitPrice = book->getHighestBuy()->getLimitPrice() + 1;
        }
    }
    if (limitPrice <= 0) limitPrice = 1;

    file << "ModifyLimit " << targetId << " " << shares << " " << limitPrice << "\n";
    book->modifyLimitOrder(targetId, shares, limitPrice);
}

void GenerateOrders::addLimitMarket()
{
    std::uniform_int_distribution<> sharesDist(1, 1000);
    std::uniform_int_distribution<> buyOrSellDist(0, 1);
    int centerPrice = 300;
    if (book->getHighestBuy() != nullptr && book->getLowestSell() != nullptr) {
        centerPrice = (book->getHighestBuy()->getLimitPrice() + book->getLowestSell()->getLimitPrice()) / 2;
    }

    int shares = sharesDist(gen);
    int limitPrice;
    bool buyOrSell = buyOrSellDist(gen);

    if (buyOrSell)
    {
        limitPrice = (book->getLowestSell() != nullptr ? book->getLowestSell()->getLimitPrice() : centerPrice) + 1;
    } else {
        limitPrice = (book->getHighestBuy() != nullptr ? book->getHighestBuy()->getLimitPrice() : centerPrice) - 1;
    }
    if (limitPrice <= 0) limitPrice = 1;
    
    file << "AddMarketLimit " << orderId << " " << buyOrSell << " " << shares << " " << limitPrice << "\n";
    book->addLimitOrder(orderId, buyOrSell, shares, limitPrice);
    orderId++;
}

void GenerateOrders::addStop()
{
    int centerPrice = 300;
    if (book->getHighestBuy() != nullptr && book->getLowestSell() != nullptr) {
        centerPrice = (book->getHighestBuy()->getLimitPrice() + book->getLowestSell()->getLimitPrice()) / 2;
    }
    std::uniform_int_distribution<> sharesDist(1, 1000);
    std::normal_distribution<> stopPriceDist(centerPrice, 30);
    std::uniform_int_distribution<> buyOrSellDist(0, 1);

    int shares = sharesDist(gen);
    bool buyOrSell = buyOrSellDist(gen);
    int stopPrice = static_cast<int>(stopPriceDist(gen));

    if (buyOrSell)
    {
        if (book->getLowestSell() != nullptr && stopPrice <= book->getLowestSell()->getLimitPrice()) {
            stopPrice = book->getLowestSell()->getLimitPrice() + 1;
        }
    } else {
        if (book->getHighestBuy() != nullptr && stopPrice >= book->getHighestBuy()->getLimitPrice()) {
            stopPrice = book->getHighestBuy()->getLimitPrice() - 1;
        }
    }
    if (stopPrice <= 0) stopPrice = 1;

    file << "AddStop " << orderId << " " << buyOrSell << " " << shares << " " << stopPrice << "\n";
    book->addStopOrder(orderId, buyOrSell, shares, stopPrice);
    activeStopOrders.push_back(orderId);
    orderId++;
}

void GenerateOrders::cancelStop()
{
    if (activeStopOrders.size() <= 500)
    {
        addStop();
        return;
    }

    std::uniform_int_distribution<> dist(0, activeStopOrders.size() - 1);
    int idx = dist(gen);
    int targetId = activeStopOrders[idx];
    activeStopOrders[idx] = activeStopOrders.back();
    activeStopOrders.pop_back();

    Order* order = book->searchOrderMap(targetId);
    if (order == nullptr)
    {
        addStop();
        return;
    }

    file << "CancelStop " << targetId << "\n";
    book->cancelStopOrder(targetId);
}

void GenerateOrders::modifyStop()
{
    if (activeStopOrders.empty())
    {
        addStop();
        return;
    }

    int centerPrice = 300;
    if (book->getHighestBuy() != nullptr && book->getLowestSell() != nullptr) {
        centerPrice = (book->getHighestBuy()->getLimitPrice() + book->getLowestSell()->getLimitPrice()) / 2;
    }
    std::uniform_int_distribution<> sharesDist(1, 1000);
    std::normal_distribution<> stopPriceDist(centerPrice, 30);

    int shares = sharesDist(gen);

    std::uniform_int_distribution<> dist(0, activeStopOrders.size() - 1);
    int idx = dist(gen);
    int targetId = activeStopOrders[idx];

    Order* order = book->searchOrderMap(targetId);
    if (order == nullptr)
    {
        activeStopOrders[idx] = activeStopOrders.back();
        activeStopOrders.pop_back();
        addStop();
        return;
    }

    bool buyOrSell = order->getBuyOrSell();
    int stopPrice = static_cast<int>(stopPriceDist(gen));

    if (buyOrSell)
    {
        if (book->getLowestSell() != nullptr && stopPrice <= book->getLowestSell()->getLimitPrice()) {
            stopPrice = book->getLowestSell()->getLimitPrice() + 1;
        }
    } else {
        if (book->getHighestBuy() != nullptr && stopPrice >= book->getHighestBuy()->getLimitPrice()) {
            stopPrice = book->getHighestBuy()->getLimitPrice() - 1;
        }
    }
    if (stopPrice <= 0) stopPrice = 1;

    file << "ModifyStop " << targetId << " " << shares << " " << stopPrice << "\n";
    book->modifyStopOrder(targetId, shares, stopPrice);
}

void GenerateOrders::addStopLimit()
{
    int centerPrice = 300;
    if (book->getHighestBuy() != nullptr && book->getLowestSell() != nullptr) {
        centerPrice = (book->getHighestBuy()->getLimitPrice() + book->getLowestSell()->getLimitPrice()) / 2;
    }
    std::uniform_int_distribution<> sharesDist(1, 1000);
    std::normal_distribution<> stopPriceDist(centerPrice, 30);
    std::uniform_int_distribution<> buyOrSellDist(0, 1);

    int shares = sharesDist(gen);
    bool buyOrSell = buyOrSellDist(gen);
    int stopPrice = static_cast<int>(stopPriceDist(gen));
    int limitPrice;

    if (buyOrSell)
    {
        if (book->getLowestSell() != nullptr && stopPrice <= book->getLowestSell()->getLimitPrice()) {
            stopPrice = book->getLowestSell()->getLimitPrice() + 1;
        }
        limitPrice = stopPrice + static_cast<int>(gen() % 5 + 1);
    } else {
        if (book->getHighestBuy() != nullptr && stopPrice >= book->getHighestBuy()->getLimitPrice()) {
            stopPrice = book->getHighestBuy()->getLimitPrice() - 1;
        }
        limitPrice = std::max(1, stopPrice - static_cast<int>(gen() % 5 + 1));
    }
    if (stopPrice <= 0) stopPrice = 1;

    file << "AddStopLimit " << orderId << " " << buyOrSell << " " << shares << " " << limitPrice << " " << stopPrice << "\n";
    book->addStopLimitOrder(orderId, buyOrSell, shares, limitPrice, stopPrice);
    activeStopLimitOrders.push_back(orderId);
    orderId++;
}

void GenerateOrders::cancelStopLimit()
{
    if (activeStopLimitOrders.size() <= 500)
    {
        addStopLimit();
        return;
    }

    std::uniform_int_distribution<> dist(0, activeStopLimitOrders.size() - 1);
    int idx = dist(gen);
    int targetId = activeStopLimitOrders[idx];
    activeStopLimitOrders[idx] = activeStopLimitOrders.back();
    activeStopLimitOrders.pop_back();

    Order* order = book->searchOrderMap(targetId);
    if (order == nullptr)
    {
        addStopLimit();
        return;
    }

    file << "CancelStopLimit " << targetId << "\n";
    book->cancelStopLimitOrder(targetId);
}

void GenerateOrders::modifyStopLimit()
{
    if (activeStopLimitOrders.empty())
    {
        addStopLimit();
        return;
    }

    int centerPrice = 300;
    if (book->getHighestBuy() != nullptr && book->getLowestSell() != nullptr) {
        centerPrice = (book->getHighestBuy()->getLimitPrice() + book->getLowestSell()->getLimitPrice()) / 2;
    }
    std::uniform_int_distribution<> sharesDist(1, 1000);
    std::normal_distribution<> stopPriceDist(centerPrice, 30);

    int shares = sharesDist(gen);

    std::uniform_int_distribution<> dist(0, activeStopLimitOrders.size() - 1);
    int idx = dist(gen);
    int targetId = activeStopLimitOrders[idx];

    Order* order = book->searchOrderMap(targetId);
    if (order == nullptr)
    {
        activeStopLimitOrders[idx] = activeStopLimitOrders.back();
        activeStopLimitOrders.pop_back();
        addStopLimit();
        return;
    }

    bool buyOrSell = order->getBuyOrSell();
    int stopPrice = static_cast<int>(stopPriceDist(gen));
    int limitPrice;

    if (buyOrSell)
    {
        if (book->getLowestSell() != nullptr && stopPrice <= book->getLowestSell()->getLimitPrice()) {
            stopPrice = book->getLowestSell()->getLimitPrice() + 1;
        }
        limitPrice = stopPrice + static_cast<int>(gen() % 5 + 1);
    } else {
        if (book->getHighestBuy() != nullptr && stopPrice >= book->getHighestBuy()->getLimitPrice()) {
            stopPrice = book->getHighestBuy()->getLimitPrice() - 1;
        }
        limitPrice = std::max(1, stopPrice - static_cast<int>(gen() % 5 + 1));
    }
    if (stopPrice <= 0) stopPrice = 1;

    file << "ModifyStopLimit " << targetId << " " << shares << " " << limitPrice << " " << stopPrice << "\n";
    book->modifyStopLimitOrder(targetId, shares, limitPrice, stopPrice);
}

void GenerateOrders::createOrders(int numberOfOrders)
{
    file.open("Orders.txt");

    if (!file.is_open()) {
        std::cerr << "Error opening file for writing!" << std::endl;
        return;
    }

    // Populate initial active orders if needed
    if (activeLimitOrders.empty()) {
        activeLimitOrders.reserve(15000);
        for (int i = 1; i <= 10000; ++i) {
            activeLimitOrders.push_back(i);
        }
    }
    if (activeStopOrders.empty() && activeStopLimitOrders.empty()) {
        activeStopOrders.reserve(1000);
        activeStopLimitOrders.reserve(1000);
        for (int i = 10001; i <= 11000; ++i) {
            if (i % 2 == 0) activeStopOrders.push_back(i);
            else activeStopLimitOrders.push_back(i);
        }
    }

    std::uniform_real_distribution<> dis(0.0, 1.0);

    // Probabilities matching the paper:
    // market: 0.025, addLimit: 0, cancelLimit: 0.195, modifyLimit: 0.295, addLimitMarket: 0.025,
    // addStop: 0, cancelStop: 0.12, modifyStop: 0.12, addStopLimit: 0, cancelStopLimit: 0.12, modifyStopLimit: 0.12
    std::vector<double> probabilities = {0.025, 0, 0.195, 0.295, 0.025, 0, 0.12, 0.12, 0, 0.12, 0.12};
    std::vector<std::function<void()>> actions = {
        std::bind(&GenerateOrders::market, this),
        std::bind(&GenerateOrders::addLimit, this),
        std::bind(&GenerateOrders::cancelLimit, this),
        std::bind(&GenerateOrders::modifyLimit, this),
        std::bind(&GenerateOrders::addLimitMarket, this),
        std::bind(&GenerateOrders::addStop, this),
        std::bind(&GenerateOrders::cancelStop, this),
        std::bind(&GenerateOrders::modifyStop, this),
        std::bind(&GenerateOrders::addStopLimit, this),
        std::bind(&GenerateOrders::cancelStopLimit, this),
        std::bind(&GenerateOrders::modifyStopLimit, this),
    };

    std::partial_sum(probabilities.begin(), probabilities.end(), probabilities.begin());

    for (size_t i = 1; i <= static_cast<size_t>(numberOfOrders); i++)
    {
        double randNum = dis(gen);
        auto it = std::lower_bound(probabilities.begin(), probabilities.end(), randNum);
        int selectedAction = std::distance(probabilities.begin(), it);

        if (selectedAction < static_cast<int>(probabilities.size())) {
            actions[selectedAction]();
        } else {
            std::cerr << "Error: No action selected!" << std::endl;
        }
    }
    file.close();
    std::cout << "Orders written to Orders.txt successfully!" << std::endl;
}

void GenerateOrders::createInitialOrders(int numberOfOrders, int centreOfBook)
{
    std::ofstream initFile("initialOrders.txt");

    if (!initFile.is_open()) {
        std::cerr << "Error opening file for writing!" << std::endl;
        return;
    }

    activeLimitOrders.clear();
    activeStopOrders.clear();
    activeStopLimitOrders.clear();
    activeLimitOrders.reserve(numberOfOrders + 1000);

    std::uniform_int_distribution<> sharesDist(1, 1000);
    std::normal_distribution<> limitPriceDist(centreOfBook, 50);

    for (int order = 1; order <= numberOfOrders; ++order) {
        int shares = sharesDist(gen);
        int limitPrice = static_cast<int>(limitPriceDist(gen));
        if (limitPrice <= 0) limitPrice = 1;
        bool buyOrSell = limitPrice < centreOfBook;

        initFile << "AddLimit " << order << " " << buyOrSell << " " << shares << " " << limitPrice << "\n";
        activeLimitOrders.push_back(order);
    }

    std::uniform_int_distribution<> stopLimitPriceDist(1, 5);
    std::uniform_int_distribution<> stopOrStopLimitDist(0, 1);

    for (int order = numberOfOrders + 1; order <= static_cast<int>(numberOfOrders * 1.1); ++order) {
        int shares = sharesDist(gen);
        int stopPrice = static_cast<int>(limitPriceDist(gen));
        if (stopPrice <= 0) stopPrice = 1;
        bool buyOrSell = stopPrice > centreOfBook;
        bool stopOrStopLimit = stopOrStopLimitDist(gen);

        if (stopOrStopLimit)
        {
            initFile << "AddStop " << order << " " << buyOrSell << " " << shares << " " << stopPrice << "\n";
            activeStopOrders.push_back(order);
        } else {
            int limitPrice;
            if (buyOrSell)
            {
                limitPrice = stopPrice + stopLimitPriceDist(gen);
            } else {
                limitPrice = std::max(1, stopPrice - static_cast<int>(stopLimitPriceDist(gen)));
            }
            initFile << "AddStopLimit " << order << " " << buyOrSell << " " << shares << " " << limitPrice << " " << stopPrice << "\n";
            activeStopLimitOrders.push_back(order);
        }
    }

    initFile.close();
    std::cout << "Orders written to initialOrders.txt successfully!" << std::endl;
}