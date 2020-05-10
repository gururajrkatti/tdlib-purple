#include "fixture.h"
#include <glib/gstrfuncs.h>

class GroupChatTest: public CommTest {
protected:
    const int32_t     groupId        = 700;
    const int64_t     groupChatId    = 7000;
    const std::string groupChatTitle = "Title";

    void loginWithBasicGroup();
};

void GroupChatTest::loginWithBasicGroup()
{
    login(
        {
            make_object<updateUser>(makeUser(
                // Chat member
                userIds[0],
                userFirstNames[0],
                userLastNames[0],
                "",
                make_object<userStatusOffline>()
            )),
            make_object<updateBasicGroup>(make_object<basicGroup>(
                groupId, 2, make_object<chatMemberStatusMember>(), true, 0
            )),
            make_object<updateNewChat>(makeChat(
                groupChatId, make_object<chatTypeBasicGroup>(groupId), groupChatTitle, nullptr, 0, 0, 0
            ))
        },
        make_object<users>(),
        make_object<chats>(std::vector<int64_t>(1, groupChatId)),
        {},
        {
            std::make_unique<ConnectionSetStateEvent>(connection, PURPLE_CONNECTED),
            std::make_unique<AddChatEvent>(
                "chat" + std::to_string(groupChatId), groupChatTitle, account, nullptr, nullptr
            ),
            std::make_unique<AccountSetAliasEvent>(account, selfFirstName + " " + selfLastName),
            std::make_unique<ShowAccountEvent>(account)
        }
    );
}

TEST_F(GroupChatTest, AddBasicGroupChatAtLogin)
{
    loginWithBasicGroup();
}

TEST_F(GroupChatTest, AddBasicGroupChat)
{
    login();
    tgl.update(make_object<updateBasicGroup>(make_object<basicGroup>(
        groupId, 2, make_object<chatMemberStatusMember>(), true, 0
    )));
    tgl.update(make_object<updateNewChat>(makeChat(
        groupChatId, make_object<chatTypeBasicGroup>(groupId), groupChatTitle, nullptr, 0, 0, 0
    )));

    prpl.verifyEvents(AddChatEvent(
        "chat" + std::to_string(groupChatId), groupChatTitle, account, NULL, NULL
    ));
}

TEST_F(GroupChatTest, ExistingBasicGroupChatAtLogin)
{
    GHashTable *components = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, g_free);
    g_hash_table_insert(components, (char *)"id", g_strdup(("chat" + std::to_string(groupChatId)).c_str()));
    purple_blist_add_chat(purple_chat_new(account, groupChatTitle.c_str(), components), NULL, NULL);
    prpl.discardEvents();

    login(
        {
            make_object<updateBasicGroup>(make_object<basicGroup>(
                groupId, 2, make_object<chatMemberStatusMember>(), true, 0
            )),
            make_object<updateNewChat>(makeChat(
                groupChatId, make_object<chatTypeBasicGroup>(groupId), groupChatTitle, nullptr, 0, 0, 0
            ))
        },
        make_object<users>(),
        make_object<chats>(std::vector<int64_t>(1, groupChatId))
    );
}

TEST_F(GroupChatTest, BasicGroupReceiveText)
{
    constexpr int32_t date[]      = {12345, 123456};
    constexpr int64_t messageId[] = {10000, 10001};
    loginWithBasicGroup();

    tgl.update(make_object<updateNewMessage>(
        makeMessage(messageId[0], userIds[0], groupChatId, false, date[0], makeTextMessage("Hello"))
    ));
    tgl.verifyRequest(viewMessages(
        groupChatId,
        {messageId[0]},
        true
    ));
    prpl.verifyEvents(
        ServGotJoinedChatEvent(connection, 1, "chat" + std::to_string(groupChatId), groupChatTitle),
        ServGotChatEvent(connection, 1, userFirstNames[0] + " " + userLastNames[0],
                         "Hello", PURPLE_MESSAGE_RECV, date[0])
    );

    tgl.update(make_object<updateNewMessage>(
        makeMessage(messageId[1], selfId, groupChatId, true, date[1], makeTextMessage("Reply"))
    ));
    tgl.verifyRequest(viewMessages(
        groupChatId,
        {messageId[1]},
        true
    ));
    prpl.verifyEvents(ConversationWriteEvent(
        "chat" + std::to_string(groupChatId), selfFirstName + " " + selfLastName,
        "Reply", PURPLE_MESSAGE_SEND, date[1]
    ));

}

TEST_F(GroupChatTest, BasicGroupReceivePhoto)
{
    const int32_t date      = 12345;
    const int64_t messageId = 10001;
    const int32_t fileId    = 1234;
    loginWithBasicGroup();

    std::vector<object_ptr<photoSize>> sizes;
    sizes.push_back(make_object<photoSize>(
        "whatever",
        make_object<file>(
            fileId, 10000, 10000,
            make_object<localFile>("", true, true, false, false, 0, 0, 0),
            make_object<remoteFile>("beh", "bleh", false, true, 10000)
        ),
        640, 480
    ));
    tgl.update(make_object<updateNewMessage>(makeMessage(
        messageId, userIds[0], groupChatId, false, date,
        make_object<messagePhoto>(
            make_object<photo>(false, nullptr, std::move(sizes)),
            make_object<formattedText>("photo", std::vector<object_ptr<textEntity>>()),
            false
        )
    )));
    tgl.verifyRequests({
        make_object<viewMessages>(groupChatId, std::vector<int64_t>(1, messageId), true),
        make_object<downloadFile>(fileId, 1, 0, 0, true)
    });
    prpl.verifyEvents(
        ServGotJoinedChatEvent(connection, 1, "chat" + std::to_string(groupChatId), groupChatTitle),
        ServGotChatEvent(connection, 1, userFirstNames[0] + " " + userLastNames[0], "photo",
                         PURPLE_MESSAGE_RECV, date),
        ConversationWriteEvent("chat" + std::to_string(groupChatId), "", "Downloading image",
                               PURPLE_MESSAGE_SYSTEM, date)
    );

    tgl.reply(make_object<ok>());
    tgl.reply(make_object<file>(
        fileId, 10000, 10000,
        make_object<localFile>("/path", true, true, false, true, 0, 10000, 10000),
        make_object<remoteFile>("beh", "bleh", false, true, 10000)
    ));
    prpl.verifyEvents(ServGotChatEvent(
        connection, 1, userFirstNames[0] + " " + userLastNames[0], "<img src=\"file:///path\">",
        PURPLE_MESSAGE_RECV, date
    ));
}

TEST_F(GroupChatTest, BasicGroupReceiveMessageAtLogin)
{
    constexpr int64_t messageId = 10001;
    constexpr int32_t date      = 12345;

    GHashTable *table = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, g_free);
    g_hash_table_insert(table, (char *)"id", g_strdup(("chat" + std::to_string(groupChatId)).c_str()));
    purple_blist_add_chat(purple_chat_new(account, groupChatTitle.c_str(), table), NULL, NULL);

    purple_blist_add_buddy(purple_buddy_new(account, userPhones[0].c_str(),
                                            (userFirstNames[0] + " " + userLastNames[0]).c_str()),
                           NULL, NULL, NULL);
    prpl.discardEvents();

    login(
        {
            standardUpdateUser(0),
            make_object<updateBasicGroup>(make_object<basicGroup>(
                groupId, 2, make_object<chatMemberStatusMember>(), true, 0
            )),
            make_object<updateNewChat>(makeChat(
                groupChatId, make_object<chatTypeBasicGroup>(groupId), groupChatTitle, nullptr, 0, 0, 0
            )),
            make_object<updateNewMessage>(
                makeMessage(messageId, userIds[0], groupChatId, false, date, makeTextMessage("Hello"))
            )
        },
        make_object<users>(),
        make_object<chats>(std::vector<int64_t>(1, groupChatId)),
        {
            // TODO: chat title is wrong here because libpurple doesn't find the chat in contact
            // list while the contact is not online, and thus has no way of knowing the chat alias.
            // Real libpurple works like that and our mock version mirrors the behaviour.
            std::make_unique<ServGotJoinedChatEvent>(connection, 1, "chat" + std::to_string(groupChatId),
                                                     "chat" + std::to_string(groupChatId)),
            std::make_unique<ServGotChatEvent>(connection, 1, userFirstNames[0] + " " + userLastNames[0],
                                               "Hello", PURPLE_MESSAGE_RECV, date)
        },
        {},
        {make_object<viewMessages>(groupChatId, std::vector<int64_t>(1, messageId), true)}
    );
}

TEST_F(GroupChatTest, SendMessage)
{
    constexpr int64_t messageId = 10001;
    constexpr int32_t date      = 12345;

    loginWithBasicGroup();

    GHashTable *components = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, g_free);
    g_hash_table_insert(components, (char *)"id", g_strdup(("chat" + std::to_string(groupChatId)).c_str()));
    pluginInfo().join_chat(connection, components);
    g_hash_table_destroy(components);

    prpl.verifyEvents(
        ServGotJoinedChatEvent(connection, 1, "chat" + std::to_string(groupChatId), groupChatTitle),
        PresentConversationEvent("chat" + std::to_string(groupChatId))
    );
    tgl.verifyNoRequests();

    ASSERT_EQ(0, pluginInfo().chat_send(connection, 1, "message", PURPLE_MESSAGE_SEND));
    tgl.verifyRequest(td::td_api::sendMessage(
        groupChatId,
        0,
        nullptr,
        nullptr,
        make_object<inputMessageText>(
            make_object<formattedText>("message", std::vector<object_ptr<textEntity>>()),
            false,
            false
        )
    ));
    prpl.verifyNoEvents();

    tgl.update(make_object<updateNewMessage>(
        makeMessage(messageId, selfId, groupChatId, true, date, makeTextMessage("message"))
    ));
    tgl.verifyRequest(viewMessages(
        groupChatId,
        {messageId},
        true
    ));
    prpl.verifyEvents(ConversationWriteEvent(
        "chat" + std::to_string(groupChatId), selfFirstName + " " + selfLastName,
        "message", PURPLE_MESSAGE_SEND, date
    ));
}