// SPDX-FileCopyrightText: 2020 Carson Black <uhhadd@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QJsonDocument>
#include <optional>

#include <google/protobuf/util/json_util.h>

#include "core.grpc.pb.h"
#include "core.pb.h"

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

	static MessageData fromProtobuf(protocol::core::v1::Message& msg) {
		std::string jsonified;
		google::protobuf::util::MessageToJsonString(msg, &jsonified, google::protobuf::util::JsonPrintOptions{});
		auto document = QJsonDocument::fromJson(QByteArray::fromStdString(jsonified));

		qDebug() << msg.has_overrides();

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

		return MessageData {
			.text = QString::fromStdString(msg.content()),
			.authorID = msg.author_id(),
			.id = msg.location().message_id(),
			.date = QDateTime::fromTime_t(msg.created_at().seconds()),
			.actions = document["actions"],
			.embeds = document["embeds"],
			.editedAt = QDateTime(),
			.replyTo = msg.in_reply_to(),
			.overrides = overrides
		};
	}
};

typedef CarrierEvent<3,protocol::core::v1::GuildEvent_MessageSent> MessageSentEvent;
typedef CarrierEvent<4,protocol::core::v1::GuildEvent_MessageUpdated> MessageUpdatedEvent;
typedef CarrierEvent<5,protocol::core::v1::GuildEvent_MessageDeleted> MessageDeletedEvent;

class ChannelsModel;

class MessagesModel : public QAbstractListModel
{
	Q_OBJECT

	QString homeServer;
	quint64 guildID;
	quint64 channelID;

	QList<MessageData> messageData;

	friend class ChannelsModel;
	friend class Client;

	bool atEnd = false;
	bool isGuildOwner = false;

	Client* client;

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
		MessageCombinedAuthorIDAvatarRole
	};

protected:
	Q_INVOKABLE void customEvent(QEvent *event) override;

public:
	MessagesModel(ChannelsModel *parent, QString homeServer, quint64 guildID, quint64 channelID);
	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	QHash<int,QByteArray> roleNames() const override;
	bool canFetchMore(const QModelIndex& parent) const override;
	void fetchMore(const QModelIndex& parent) override;

	Q_INVOKABLE bool isOwner() { return isGuildOwner; }
	Q_INVOKABLE QString userID() { return QString::number(client->userID); }
	Q_INVOKABLE QVariantMap peekMessage(const QString& id);
	Q_INVOKABLE void sendMessage(const QString& content, const QString& replyTo);
	Q_INVOKABLE void editMessage(const QString& id, const QString& content);
	Q_INVOKABLE void deleteMessage(const QString& id);
	Q_INVOKABLE void triggerAction(const QString& messageID, const QString& name, const QString& data);
};
