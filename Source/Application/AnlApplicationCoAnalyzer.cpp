#include "AnlApplicationCoAnalyzer.h"
#include "AnlApplicationInstance.h"
#include <AnlIconsData.h>

ANALYSE_FILE_BEGIN

Application::CoAnalyzerContent::CoAnalyzerContent()
: mSendButton(juce::ImageCache::getFromMemory(AnlIconsData::play_png, AnlIconsData::play_pngSize))
{
    mQueryEditor.setMultiLine(true);
    mQueryEditor.setReturnKeyStartsNewLine(false);
    mQueryEditor.setScrollbarsShown(true);
    mQueryEditor.setCaretVisible(true);
    mQueryEditor.setPopupMenuEnabled(true);
    mQueryEditor.setFont(juce::Font(juce::FontOptions(14.0f)));
    mQueryEditor.setTextToShowWhenEmpty(juce::translate("Enter your query in natural language..."), juce::Colours::grey);
    mQueryEditor.onReturnKey = [this]()
    {
        sendRequest();
    };

    mSendButton.setTooltip(juce::translate("Send Query"));
    mSendButton.onClick = [this]()
    {
        sendRequest();
    };

    mApiKeyButton.setButtonText(juce::translate("API Key"));
    mApiKeyButton.setTooltip(juce::translate("Set Mistral API Key"));
    mApiKeyButton.onClick = [this]()
    {
        showApiKeyDialog();
    };

    mStatusLabel.setJustificationType(juce::Justification::centredLeft);
    mStatusLabel.setFont(juce::Font(juce::FontOptions(12.0f)));
    mStatusLabel.setColour(juce::Label::textColourId, juce::Colours::grey);

    addAndMakeVisible(mQueryEditor);
    addAndMakeVisible(mSendButton);
    addAndMakeVisible(mApiKeyButton);
    addAndMakeVisible(mStatusLabel);
}

Application::CoAnalyzerContent::~CoAnalyzerContent()
{
    if(mRequestFuture.valid())
    {
        mRequestFuture.wait();
    }
}

void Application::CoAnalyzerContent::paint(juce::Graphics& g)
{
    g.fillAll(findColour(juce::ResizableWindow::backgroundColourId));
}

void Application::CoAnalyzerContent::resized()
{
    auto bounds = getLocalBounds().reduced(4);
    auto topBar = bounds.removeFromTop(32);

    mApiKeyButton.setBounds(topBar.removeFromRight(80));
    topBar.removeFromRight(4);
    mSendButton.setBounds(topBar.removeFromRight(32));
    topBar.removeFromRight(4);

    bounds.removeFromTop(4);
    auto bottomBar = bounds.removeFromBottom(20);
    mStatusLabel.setBounds(bottomBar);

    bounds.removeFromBottom(4);
    mQueryEditor.setBounds(bounds);
}

void Application::CoAnalyzerContent::parentHierarchyChanged()
{
    mQueryEditor.applyFontToAllText(mQueryEditor.getFont());
}

void Application::CoAnalyzerContent::handleAsyncUpdate()
{
    if(!mRequestPending || !mRequestFuture.valid())
    {
        return;
    }

    if(mRequestFuture.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
    {
        return;
    }

    auto [result, xmlElement] = mRequestFuture.get();
    mRequestPending = false;

    if(result.failed())
    {
        mStatusLabel.setText(juce::translate("Error: ") + result.getErrorMessage(), juce::dontSendNotification);
        mQueryEditor.setEnabled(true);
        mSendButton.setEnabled(true);
        mApiKeyButton.setEnabled(true);
        return;
    }

    if(xmlElement == nullptr)
    {
        mStatusLabel.setText(juce::translate("Error: Invalid XML response"), juce::dontSendNotification);
        mQueryEditor.setEnabled(true);
        mSendButton.setEnabled(true);
        mApiKeyButton.setEnabled(true);
        return;
    }

    // Apply the XML to the document
    auto& director = Instance::get().getDocumentDirector();
    auto& accessor = director.getAccessor();

    director.startAction();
    accessor.fromXml(*xmlElement, "Document", NotificationType::synchronous);
    director.endAction(ActionState::newTransaction, juce::translate("Co-Analyzer modification"));

    mStatusLabel.setText(juce::translate("Document updated successfully"), juce::dontSendNotification);
    mQueryEditor.setEnabled(true);
    mSendButton.setEnabled(true);
    mApiKeyButton.setEnabled(true);
    mQueryEditor.clear();
}

void Application::CoAnalyzerContent::sendRequest()
{
    if(mRequestPending)
    {
        return;
    }

    auto const query = mQueryEditor.getText();
    if(query.isEmpty())
    {
        mStatusLabel.setText(juce::translate("Please enter a query"), juce::dontSendNotification);
        return;
    }

    if(mApiKey.isEmpty())
    {
        mStatusLabel.setText(juce::translate("Please set your API key first"), juce::dontSendNotification);
        showApiKeyDialog();
        return;
    }

    // Get current document XML
    auto& accessor = Instance::get().getDocumentAccessor();
    auto xmlElement = accessor.toXml("Document");
    if(xmlElement == nullptr)
    {
        mStatusLabel.setText(juce::translate("Error: Could not export document"), juce::dontSendNotification);
        return;
    }

    auto const xmlData = xmlElement->toString();

    mQueryEditor.setEnabled(false);
    mSendButton.setEnabled(false);
    mApiKeyButton.setEnabled(false);
    mStatusLabel.setText(juce::translate("Sending request..."), juce::dontSendNotification);
    mRequestPending = true;

    // Launch async request
    mRequestFuture = std::async(std::launch::async, [this, apiKey = mApiKey, xmlData, query]()
                                {
                                    auto result = sendRequestToMistral(apiKey, xmlData, query);
                                    triggerAsyncUpdate();
                                    return result;
                                });
}

void Application::CoAnalyzerContent::showApiKeyDialog()
{
    auto alertWindow = std::make_unique<juce::AlertWindow>(
        juce::translate("Mistral API Key"),
        juce::translate("Enter your Mistral API key:"),
        juce::AlertWindow::AlertIconType::NoIcon);

    alertWindow->addTextEditor("apiKey", mApiKey.isEmpty() ? juce::SystemClipboard::getTextFromClipboard() : mApiKey, juce::translate("API Key"));
    alertWindow->getTextEditor("apiKey")->setInputRestrictions(0, "");

    alertWindow->addButton(juce::translate("OK"), 1, juce::KeyPress(juce::KeyPress::returnKey));
    alertWindow->addButton(juce::translate("Cancel"), 0, juce::KeyPress(juce::KeyPress::escapeKey));

    auto* rawPtr = alertWindow.get();
    alertWindow->enterModalState(true, juce::ModalCallbackFunction::create([this, rawPtr](int result)
                                                                           {
                                                                               if(result == 1)
                                                                               {
                                                                                   mApiKey = rawPtr->getTextEditorContents("apiKey");
                                                                                   if(!mApiKey.isEmpty())
                                                                                   {
                                                                                       mStatusLabel.setText(juce::translate("API key set"), juce::dontSendNotification);
                                                                                   }
                                                                               }
                                                                           }),
                                 false);
    alertWindow.release(); // Release ownership as JUCE will handle deletion
}

std::tuple<juce::Result, std::unique_ptr<juce::XmlElement>> Application::CoAnalyzerContent::sendRequestToMistral(
    juce::String const& apiKey, juce::String const& xmlData, juce::String const& userPrompt)
{
    // Prepare the JSON payload for Mistral API
    auto escapedXmlData = xmlData.replace("\"", "\\\"").replace("\n", "\\n");
    auto escapedUserPrompt = userPrompt.replace("\"", "\\\"").replace("\n", "\\n");

    juce::String payload = juce::String::formatted(
        R"({"model":"mistral-small-latest","messages":[{"role":"system","content":"You are an AI assistant for the Partiels audio analysis application. You will receive XML data representing the current document state and a user request. Generate ONLY the modified XML document without any additional text, explanations, or markdown formatting. The XML should be well-formed and represent the complete document structure."},{"role":"user","content":"Current document XML:\n%s\n\nUser request: %s\n\nGenerate the modified XML:"}],"temperature":0.3})",
        escapedXmlData.toRawUTF8(),
        escapedUserPrompt.toRawUTF8());

    juce::URL url("https://api.mistral.ai/v1/chat/completions");
    url = url.withPOSTData(payload);

    juce::String headers;
    headers << "Content-Type: application/json\r\n";
    headers << "Authorization: Bearer " << apiKey << "\r\n";

    auto stream = url.createInputStream(
        juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
            .withExtraHeaders(headers)
            .withConnectionTimeoutMs(30000)
            .withNumRedirectsToFollow(5));

    if(stream == nullptr)
    {
        return {juce::Result::fail(juce::translate("Failed to connect to Mistral API")), nullptr};
    }

    juce::String response = stream->readEntireStreamAsString();
    juce::Logger::writeToLog("Mistral API Response: " + response);

    auto jsonResponse = juce::JSON::parse(response);
    if(jsonResponse.isVoid() || !jsonResponse.isObject())
    {
        return {juce::Result::fail(juce::translate("Invalid JSON response from API")), nullptr};
    }

    auto* jsonObj = jsonResponse.getDynamicObject();
    if(jsonObj == nullptr)
    {
        return {juce::Result::fail(juce::translate("Invalid response structure")), nullptr};
    }

    // Check for API errors
    if(jsonObj->hasProperty("error"))
    {
        auto errorObj = jsonObj->getProperty("error");
        if(errorObj.isObject() && errorObj.getDynamicObject()->hasProperty("message"))
        {
            return {juce::Result::fail(errorObj.getDynamicObject()->getProperty("message").toString()), nullptr};
        }
        return {juce::Result::fail(juce::translate("API returned an error")), nullptr};
    }

    // Extract the generated content
    auto choicesArray = jsonObj->getProperty("choices");
    if(!choicesArray.isArray() || choicesArray.getArray()->size() == 0)
    {
        return {juce::Result::fail(juce::translate("No response choices returned")), nullptr};
    }

    auto firstChoice = choicesArray.getArray()->getReference(0);
    if(!firstChoice.isObject())
    {
        return {juce::Result::fail(juce::translate("Invalid choice format")), nullptr};
    }

    auto messageObj = firstChoice.getDynamicObject()->getProperty("message");
    if(!messageObj.isObject())
    {
        return {juce::Result::fail(juce::translate("Invalid message format")), nullptr};
    }

    auto content = messageObj.getDynamicObject()->getProperty("content").toString();
    if(content.isEmpty())
    {
        return {juce::Result::fail(juce::translate("Empty response content")), nullptr};
    }

    // Remove markdown code blocks if present
    content = content.trim();
    if(content.startsWith("```xml"))
    {
        content = content.substring(6);
    }
    else if(content.startsWith("```"))
    {
        content = content.substring(3);
    }

    if(content.endsWith("```"))
    {
        content = content.dropLastCharacters(3);
    }
    content = content.trim();

    // Parse the XML
    auto xmlElement = juce::parseXML(content);
    if(xmlElement == nullptr)
    {
        juce::Logger::writeToLog("Failed to parse XML content: " + content);
        return {juce::Result::fail(juce::translate("Failed to parse XML response")), nullptr};
    }

    return {juce::Result::ok(), std::move(xmlElement)};
}

Application::CoAnalyzerPanel::CoAnalyzerPanel()
: mTitleLabel("Title", juce::translate("Co-Analyzer"))
{
    mTitleLabel.setJustificationType(juce::Justification::centredLeft);
    mTitleLabel.setFont(juce::Font(juce::FontOptions(14.0f).withStyle("Bold")));

    addAndMakeVisible(mTitleLabel);
    addAndMakeVisible(mTopSeparator);
    addAndMakeVisible(mContent);
}

Application::CoAnalyzerPanel::~CoAnalyzerPanel()
{
}

void Application::CoAnalyzerPanel::paint(juce::Graphics& g)
{
    g.fillAll(findColour(juce::ResizableWindow::backgroundColourId));
}

void Application::CoAnalyzerPanel::resized()
{
    auto bounds = getLocalBounds();
    auto top = bounds.removeFromTop(28);
    top.removeFromLeft(8);
    mTitleLabel.setBounds(top);
    mTopSeparator.setBounds(bounds.removeFromTop(1));
    mContent.setBounds(bounds);
}

void Application::CoAnalyzerPanel::colourChanged()
{
    setOpaque(findColour(juce::ResizableWindow::backgroundColourId).isOpaque());
}

void Application::CoAnalyzerPanel::parentHierarchyChanged()
{
    colourChanged();
}

ANALYSE_FILE_END
