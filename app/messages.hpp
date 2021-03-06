// SPDX-FileCopyrightText: 2020 Carson Black <uhhadd@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QJsonDocument>
#include <QJSValue>
#include <QQmlPropertyMap>
#include <optional>

#include <google/protobuf/util/json_util.h>

#include "chat/v1/chat.grpc.pb.h"
#include "chat/v1/chat.pb.h"

#include "client.hpp"
#include "util.hpp"

struct MessageData
{
	QString text;
	quint64 authorID;
	quint64 id;
	QDateTime date;
	QJsonValue actions;
	QJsonValue embeds;
	QDateTime editedAt;
	quint64 replyTo;

	struct Override
	{
		QString name;
		QString avatar;

		enum Reason {
			Plurality,
			Bridge,
			Webhook
		};
		Reason reason;
	};
	std::optional<Override> overrides;

	QVariantList attachments;

	enum State {
		Sent,
		Sending,
		Failed,
	};
	State status;
	quint64 echoID;

	static MessageData fromProtobuf(protocol::harmonytypes::v1::Message& msg) {
		std::string jsonified;
		google::protobuf::util::MessageToJsonString(msg, &jsonified, google::protobuf::util::JsonPrintOptions{});
		auto document = QJsonDocument::fromJson(QByteArray::fromStdString(jsonified));

		std::optional<Override> overrides;
		if (msg.has_overrides()) {
			overrides = Override{};
			overrides->name = QString::fromStdString(msg.overrides().name());
			overrides->avatar = QString::fromStdString(msg.overrides().avatar());
			if (msg.overrides().has_system_plurality()) {
				overrides->reason = Override::Plurality;
			} else if (msg.overrides().has_webhook()) {
				overrides->reason = Override::Webhook;
			} else if (msg.overrides().has_bridge()) {
				overrides->reason = Override::Bridge;
			}
		}

		auto msgAttaches = msg.attachments();
		QVariantList attachments;
		for (auto attach : msgAttaches) {
			std::string jsonified;
			google::protobuf::util::MessageToJsonString(attach, &jsonified, google::protobuf::util::JsonPrintOptions{});
			auto document = QJsonDocument::fromJson(QByteArray::fromStdString(jsonified));

			attachments << document.object();
		}

		return MessageData {
			.text = QString::fromStdString(msg.content()),
			.authorID = msg.author_id(),
			.id = msg.message_id(),
			.date = QDateTime::fromTime_t(msg.created_at().seconds()),
			.actions = document["actions"],
			.embeds = document["embeds"],
			.editedAt = QDateTime(),
			.replyTo = msg.in_reply_to(),
			.overrides = overrides,
			.attachments = attachments,
			.status = State::Sent,
		};
	}
};

class ChannelsModel;
class QNetworkAccessManager;

class MessagesModel : public QAbstractListModel
{
	Q_OBJECT

	QString homeServer;
	quint64 guildID;
	quint64 channelID;

	QList<MessageData> messageData;
	QHash<quint64,MessageData*> echoes;
	QQmlPropertyMap* permissions;
	QHash<quint64,int> userCounts;

	friend class ChannelsModel;
	friend class Client;

	bool atEnd = false;
	bool isGuildOwner = false;
	bool isReadingMore = false;

	Client* client;

	Q_PROPERTY(ChannelsModel* parentModel READ channelsModel CONSTANT FINAL)
	ChannelsModel* channelsModel() { return reinterpret_cast<ChannelsModel*>(parent()); }

	Q_PROPERTY(QQmlPropertyMap* permissions MEMBER permissions CONSTANT FINAL)
	Q_PROPERTY(QString homeserver MEMBER homeServer CONSTANT FINAL)
	Q_PROPERTY(QString typingIndicator READ typingIndicator NOTIFY typingIndicatorChanged)

	enum Roles {
		MessageTextRole = Qt::UserRole,
		MessageEmbedsRole,
		MessageActionsRole,
		MessageAuthorRole,
		MessageAuthorAvatarRole,
		MessageAuthorIDRole,
		MessageShouldDisplayAuthorInfo,
		MessageDateRole,
		MessageReplyToRole,
		MessageIDRole,
		MessageAttachmentsRole,
		MessageCombinedAuthorIDAvatarRole,
		MessageQuirkRole,
		MessageModelIndexRole,
		MessageStatusRole,
	};

	struct Fronter {
		QString name;
	};
	struct RoleplayCharacter {
		QString name;
	};

	using Nobody = std::monostate;
	using SendAs = std::variant<Nobody, Fronter, RoleplayCharacter>;

	void reevaluateTypingIndicator();

protected:
	Q_INVOKABLE void customEvent(QEvent *event) override;

public:
	MessagesModel(ChannelsModel *parent, QString homeServer, quint64 guildID, quint64 channelID);
	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	QHash<int,QByteArray> roleNames() const override;
	bool canFetchMore(const QModelIndex& parent) const override;
	void fetchMore(const QModelIndex& parent) override;

	QString typingIndicator();
	Q_SIGNAL void typingIndicatorChanged();

	Q_INVOKABLE void typed();
	Q_INVOKABLE bool isOwner() { return isGuildOwner; }
	Q_INVOKABLE QString userID() { return QString::number(client->userID); }
	Q_INVOKABLE QVariantMap peekMessage(const QString& id);
	Q_INVOKABLE void sendMessageFull(const QString& content, const QString& replyTo, const QStringList& attachments, const SendAs& as);
	Q_INVOKABLE void sendMessage(const QString& content, const QString& replyTo, const QStringList& attachments)
	{
		sendMessageFull(content, replyTo, attachments, SendAs(Nobody{}));
	}
	Q_INVOKABLE void sendMessageAsSystem(const QString& content, const QString& replyTo, const QStringList& attachments, const QString& memberName)
	{
		sendMessageFull(content, replyTo, attachments, SendAs(Fronter {
			.name = memberName
		}));
	}
	Q_INVOKABLE void sendMessageAsRoleplay(const QString& content, const QString& replyTo, const QStringList& attachments, const QString& characterName)
	{
		sendMessageFull(content, replyTo, attachments, SendAs(RoleplayCharacter {
			.name = characterName
		}));
	}
	Q_INVOKABLE void editMessage(const QString& id, const QString& content);
	Q_INVOKABLE void deleteMessage(const QString& id);
	Q_INVOKABLE void triggerAction(const QString& messageID, const QString& name, const QString& data);
};
