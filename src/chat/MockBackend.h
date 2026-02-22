#pragma once

#include <QObject>
#include <QTimer>

#include <wechat/network/NetworkClient.h>

#include <cstdint>
#include <string>
#include <vector>

namespace wechat::chat {

/// 模拟后端：以"对方用户"身份执行操作，走完整网络层路径
///
/// 支持两种模式：
/// 1. 预灌数据（同步）：在 ChatWidget 创建前批量写入历史消息
/// 2. 定时脚本（异步）：按时间线执行一系列动作（发消息、撤回、编辑）
///
/// 所有操作通过 ChatService 接口完成，ChatPresenter 正常感知推送通知。
class MockBackend : public QObject {
    Q_OBJECT

public:
    explicit MockBackend(network::NetworkClient& client,
                         QObject* parent = nullptr);

    /// 设置模拟用户会话
    void setPeerSession(std::string const& token,
                        int64_t userId);

    /// 设置目标聊天
    void setChatId(int64_t chatId);

    // ── 预灌数据（同步，ChatWidget 创建前调用）──

    /// 批量写入历史消息，返回写入的消息 ID 列表
    /// senderTokens 交替使用，模拟双方对话
    std::vector<int64_t> prefill(int count,
                                 std::vector<std::string> const& senderTokens);

    // ── 定时脚本（异步，ChatWidget 创建后调用）──

    /// 动作类型
    enum class Action { Send, Revoke, Edit };

    struct ScriptEntry {
        int delayMs;          // 距上一步的延迟（ms）
        Action action;
        std::string text;     // Send/Edit 的文本内容
        int64_t targetMsgId;  // Revoke/Edit 的目标消息 ID（Send 时忽略）
                              // < 0 时为索引引用：-n 表示 sentMsgIds_[n-1]
    };

    /// 设置脚本并开始执行
    void runScript(std::vector<ScriptEntry> script);

    /// 停止脚本
    void stop();

    /// 便捷：生成一个典型测试脚本
    /// 发 5 条消息 → 撤回第 2 条 → 编辑第 4 条 → 再发 3 条
    static std::vector<ScriptEntry> typicalScript();

Q_SIGNALS:
    void scriptFinished();

private:
    void executeNext();
    int64_t resolveTargetId(int64_t targetMsgId) const;

    network::NetworkClient& client_;
    std::string peerToken_;
    int64_t peerId_ = 0;
    int64_t chatId_ = 0;

    QTimer timer_;
    std::vector<ScriptEntry> script_;
    int scriptIndex_ = 0;
    std::vector<int64_t> sentMsgIds_;
};

} // namespace wechat::chat
