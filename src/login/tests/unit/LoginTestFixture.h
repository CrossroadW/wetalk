#pragma once

#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QEventLoop>
#include <QSignalSpy>
#include <QTest>
#include <QTimer>

#include <wechat/core/User.h>
#include <wechat/login/LoginPresenter.h>
#include <wechat/network/NetworkClient.h>

using namespace wechat;

Q_DECLARE_METATYPE(core::User)

/**
 * 共享的登录测试基类
 *
 * 提供 WebSocket 客户端连接、LoginPresenter 实例和辅助方法
 * 每个测试用例会自动连接后端、执行测试、重置数据库
 */
class LoginPresenterTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        // 注册自定义类型用于信号/槽
        qRegisterMetaType<core::User>();
    }

    void SetUp() override {
        // 使用 WebSocket 客户端连接真实后端
        // 如果连接失败会抛出异常，测试会失败（不是 SKIP）
        // 使用较短的超时时间加快测试速度
        try {
            client = network::createWsClient("ws://localhost:8000/ws");
            presenter = std::make_unique<login::LoginPresenter>(*client);
        } catch (const std::exception& e) {
            FAIL() << "Failed to connect to backend: " << e.what()
                   << "\nMake sure backend is running at ws://localhost:8000/ws";
        }
    }

    void TearDown() override {
        // 测试完成后重置数据库（带超时）
        QNetworkAccessManager mgr;
        QNetworkRequest req(QUrl("http://localhost:8000/api/test/reset-db"));
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        req.setTransferTimeout(2000);  // 2秒超时

        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        timer.setInterval(2000);  // 2秒超时

        auto* reply = mgr.post(req, QByteArray{});
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

        timer.start();
        loop.exec();

        reply->deleteLater();
    }

    // 辅助方法：等待信号并处理事件循环（带超时）
    bool waitForSignal(QSignalSpy& spy, int timeoutMs = 3000) {
        if (spy.count() > 0) return true;
        return spy.wait(timeoutMs);
    }

    std::unique_ptr<network::NetworkClient> client;
    std::unique_ptr<login::LoginPresenter> presenter;
};
